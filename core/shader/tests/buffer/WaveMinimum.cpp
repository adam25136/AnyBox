#include "IntegerWaveKernel.hpp"
#include "FloatFixture.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include <algorithm>

void CheckWaveReductionProof(std::span<const std::uint32_t> code);

namespace {
void Invalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 113), fixture.Integer(32, 1)}; };
    auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call("xbox.WaveMin_U32", type, std::move(values))); };
    reject(word, {});
    reject(fixture.FloatType(), arguments());
    reject(fixture.IntegerType(64), arguments());
    auto extra = arguments();
    extra.push_back(fixture.Integer(32, 0));
    reject(word, std::move(extra));
    for (unsigned index = 0; index < 2; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(64, 1);
        reject(word, std::move(values));
    }
    for (unsigned opcode : {0u, 112u, 114u}) {
        auto values = arguments();
        values[0] = fixture.Integer(32, opcode);
        reject(word, std::move(values));
    }
    auto values = arguments();
    values[0] = fixture.Make<LLVMBC::Argument>(word, 0, false);
    reject(word, std::move(values));
}

void Check(AnyBox::Vulkan::Context& context, unsigned wave, unsigned threads, std::uint32_t mask, bool native, unsigned pattern = 0, std::optional<std::uint64_t> constant = {}) {
    auto program = BufferTests::MakeIntegerWaveKernel(wave, false, mask, 1, true, 32, constant, true);
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
    unsigned reductions = 0;
    unsigned shuffles = 0;
    for (std::size_t offset = 5; offset < shader.code.size(); offset += shader.code[offset] >> 16) {
        auto opcode = shader.code[offset] & 65535;
        if (opcode == spv::OpGroupNonUniformUMin) ++reductions;
        if (opcode == spv::OpGroupNonUniformShuffle) ++shuffles;
    }
    if (constant) Fixtures::Require(reductions == 0 && shuffles == 0);
    else if (native) Fixtures::Require(reductions == 1 && shuffles == 0);
    else if (wave == 32 && mask == UINT32_MAX) CheckWaveReductionProof(shader.code);

    std::array<std::uint32_t, 64> input;
    constexpr std::array<std::uint32_t, 8> corners{UINT32_MAX, 0x80000000, 0x7FFFFFFF, 1, 0, 0xFFFFFFFF, 0xFFFF0000, 0x12345678};
    for (unsigned lane = 0; lane < input.size(); ++lane) input[lane] = pattern == 1 ? 64 - lane : pattern == 2 ? lane + 1 : corners[lane % corners.size()];
    if (pattern == 3) input.fill(UINT32_MAX);
    std::vector<std::uint32_t> expected;
    for (unsigned thread = 0; thread < threads * 2; ++thread) {
        auto local = thread % threads;
        auto lane = local % wave;
        bool active = wave == 64 || (mask & (1u << lane)) != 0;
        auto available = std::min(wave, threads - local / wave * wave);
        std::uint32_t minimum = UINT32_MAX;
        for (unsigned other = 0; other < available; ++other) {
            if (wave == 64 || (mask & (1u << other)) != 0) minimum = std::min(minimum, constant ? static_cast<std::uint32_t>(*constant) : input[other]);
        }
        expected.push_back(active ? minimum : 0xDEADBEEF);
    }
    std::vector<std::uint32_t> initial(expected.size(), 0xDEADBEEF);
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1}, std::as_bytes(std::span(input)), {}, std::as_bytes(std::span(initial)));
}
}

void CheckWaveMinimum(AnyBox::Vulkan::Context& context) {
    Invalid();
    for (unsigned pattern = 0; pattern < 4; ++pattern) {
        for (bool native : {false, true}) {
            for (auto mask : {UINT32_MAX, 0xA55A8420U, 0xFFFF0000U, 0x80000000U, 1u, 0u}) Check(context, 32, 128, mask, native, pattern);
        }
        Check(context, 64, 128, UINT32_MAX, false, pattern);
        for (unsigned wave : {32u, 64u}) {
            for (unsigned threads : {1u, 17u, 33u, 65u, 127u}) Check(context, wave, threads, UINT32_MAX, false, pattern);
        }
    }
    for (auto value : {0u, 1u, 0x80000000u, UINT32_MAX}) {
        for (unsigned wave : {32u, 64u}) Check(context, wave, 128, UINT32_MAX, wave == 32, 0, value);
    }
}
