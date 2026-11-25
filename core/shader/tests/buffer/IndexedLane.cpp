#include "UniformKernel.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

void CheckIndexedLaneInvalid();

namespace {
void Check(AnyBox::Vulkan::Context& context, unsigned bits, bool floating, unsigned wave, unsigned threads, unsigned first, bool uniform, BufferTests::IndexedLaneSelector selector) {
    auto program = BufferTests::UniformKernel(bits, wave, first, uniform, true, floating, selector);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    if (bits == 32 && !floating && !uniform && wave == 64 && threads == 128 && first == 0) {
        unsigned shuffles = 0;
        for (std::size_t offset = 5; offset < shader.code.size(); offset += shader.code[offset] >> 16) if ((shader.code[offset] & 65535) == spv::OpGroupNonUniformShuffle) ++shuffles;
        Fixtures::Require(shuffles == (selector.mode == BufferTests::LaneSelectorMode::Constant ? 2u : 4u));
    }
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto lane = index % threads % wave;
        bool active = lane >= first;
        unsigned chosen = selector.lane;
        if (selector.mode == BufferTests::LaneSelectorMode::Own) chosen = lane;
        if (selector.mode == BufferTests::LaneSelectorMode::Xor) chosen = lane ^ 1;
        if (selector.mode == BufferTests::LaneSelectorMode::Reverse) chosen = wave - 1 - lane;
        auto selected = index - lane + chosen;
        auto value = uniform ? bits == 1 ? 1u : 0xA55AC33C : bits == 1 ? static_cast<unsigned>((selected & 65) != 0) : selected * 17 + 5;
        expected.push_back(active ? value : 0xDEADBEEF);
        if (bits == 64) expected.push_back(!active ? 0xCAFEBABE : uniform ? 0x81FEDCBA : selected ^ 0xA5F03C69);
    }
    try { BufferTests::RunGpu(context, shader, expected, {2, 1, 1}); }
    catch (const std::runtime_error& error) {
        throw std::runtime_error("Indexed lane bits=" + std::to_string(bits) + " floating=" + std::to_string(floating) + " wave=" + std::to_string(wave) + " threads=" + std::to_string(threads) + " first=" + std::to_string(first) + " uniform=" + std::to_string(uniform) + " mode=" + std::to_string(static_cast<unsigned>(selector.mode)) + " lane=" + std::to_string(selector.lane) + ": " + error.what());
    }
}
}

void CheckIndexedLane(AnyBox::Vulkan::Context& context) {
    using Mode = BufferTests::LaneSelectorMode;
    using Selector = BufferTests::IndexedLaneSelector;
    CheckIndexedLaneInvalid();
    for (auto overload : {std::pair{1u, false}, {32u, false}, {64u, false}, {32u, true}}) {
        auto [bits, floating] = overload;
        for (unsigned wave : {32u, 64u}) {
            for (bool uniform : {false, true}) {
                for (auto mode : {Mode::Own, Mode::Xor, Mode::Reverse}) Check(context, bits, floating, wave, 128, 0, uniform, {mode});
                for (unsigned lane : {0u, wave - 1}) Check(context, bits, floating, wave, 128, 0, uniform, {Mode::Constant, lane});
            }
            for (unsigned threads : {1u, 17u, 33u, 65u, 127u}) {
                for (auto mode : {Mode::Own, Mode::Constant}) Check(context, bits, floating, wave, threads, 0, false, {mode});
            }
        }
        for (unsigned first : {1u, 7u, 16u, 31u, 32u}) {
            Check(context, bits, floating, 32, 128, first, false, {Mode::Own});
            Check(context, bits, floating, 32, 128, first, false, {Mode::Constant, first % 32});
        }
    }
    for (unsigned wave : {32u, 64u}) {
        for (unsigned lane = 0; lane < wave; ++lane) Check(context, 32, false, wave, 128, 0, false, {Mode::Constant, lane});
    }
    auto divergent = BufferTests::UniformKernel(32, 64, 1, false, true, false, Selector{Mode::Own});
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    Fixtures::Reject<std::runtime_error>([&] { (void)AnyBox::Shader::Dxil::CompileCompute(divergent, info, BufferTests::Target(), {0, 0, 0, 128}); });
}
