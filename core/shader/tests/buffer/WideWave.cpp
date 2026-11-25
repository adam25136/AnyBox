#include "IntegerWaveKernel.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include <algorithm>

void CheckWideWaveInvalid();

namespace {
void Check(AnyBox::Vulkan::Context& context, unsigned wave, unsigned threads, bool prefix, std::uint32_t mask, bool native, std::optional<std::uint64_t> constantValue = {}, unsigned pattern = 0) {
    auto program = BufferTests::MakeIntegerWaveKernel(wave, prefix, mask, pattern % 2, true, 64, constantValue);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto target = BufferTests::Target();
    std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    if (native) capabilities.push_back(spv::CapabilityGroupNonUniformArithmetic);
    target.supportedCapabilities = capabilities;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, target, {0, 0, 0, 128});
    unsigned shuffles = 0;
    unsigned reductions = 0;
    for (std::size_t offset = 5; offset < shader.code.size(); offset += shader.code[offset] >> 16) {
        auto opcode = shader.code[offset] & 65535;
        if (opcode == spv::OpGroupNonUniformShuffle) ++shuffles;
        if (opcode == spv::OpGroupNonUniformIAdd) ++reductions;
    }
    if (constantValue) Fixtures::Require(shuffles == 0);
    else if (native) Fixtures::Require(shuffles == 0 && reductions == 3);

    std::array<std::uint64_t, 8> input{UINT64_MAX, 1, 0xFFFFFFFF, 0x100000000, 0x80000000FFFFFFFF, 0x7FFFFFFF0000FFFF, 0xFFFF0000FFFF0000, 0x0123456789ABCDEF};
    if (pattern == 1) input.fill(UINT64_MAX);
    if (pattern == 2) input.fill(65535);
    if (pattern == 3) input.fill(0xFFFF0000);
    if (pattern == 4) input.fill(0xFFFFFFFF);
    std::vector<std::uint32_t> expected;
    for (unsigned thread = 0; thread < threads * 2; ++thread) {
        auto local = thread % threads;
        auto lane = local % wave;
        bool active = wave == 64 || (mask & (1u << lane)) != 0;
        std::uint64_t sum = 0;
        auto available = std::min(wave, threads - local / wave * wave);
        for (unsigned other = 0; other < (prefix ? lane : available); ++other) {
            if (wave == 64 || (mask & (1u << other)) != 0) sum += constantValue ? *constantValue : input[other % input.size()];
        }
        expected.push_back(active ? static_cast<std::uint32_t>(sum) : UINT32_MAX);
        expected.push_back(active ? static_cast<std::uint32_t>(sum >> 32) : UINT32_MAX);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1}, std::as_bytes(std::span(input)));
}
}

void CheckWideWaveGpu(AnyBox::Vulkan::Context& context) {
    CheckWideWaveInvalid();
    for (bool prefix : {false, true}) {
        for (bool native : {false, true}) {
            for (auto mask : {UINT32_MAX, 0xA55A8420U, 0x80000000U, 0u}) Check(context, 32, 32, prefix, mask, native);
            for (unsigned pattern = 1; pattern <= 4; ++pattern) Check(context, 32, 32, prefix, UINT32_MAX, native, {}, pattern);
        }
        Check(context, 64, 128, prefix, UINT32_MAX, false);
        for (unsigned wave : {32u, 64u}) {
            for (auto value : {std::uint64_t{0}, std::uint64_t{1}, UINT64_MAX, std::uint64_t{0xFFFFFFFF}, std::uint64_t{0x123456789ABCDEF0}}) Check(context, wave, 128, prefix, 0xA55A8420, false, value);
            for (unsigned threads : {1u, 17u, 33u, 65u, 127u}) {
                Check(context, wave, threads, prefix, UINT32_MAX, false);
                Check(context, wave, threads, prefix, UINT32_MAX, false, UINT64_MAX);
            }
        }
    }
}
