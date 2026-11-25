#include "IntegerWaveKernel.hpp"
#include "FloatFixture.hpp"
#include "Fixtures.hpp"
#include "ComputeCompiler.hpp"

void CheckIntegerWaveProof(std::span<const std::uint32_t> code, bool prefix);
void CheckWideWaveGpu(AnyBox::Vulkan::Context& context);
void CheckWaveMinimum(AnyBox::Vulkan::Context& context);

namespace {
void Check(AnyBox::Vulkan::Context& context, unsigned waveSize, bool prefix, std::uint32_t mask, bool native, unsigned signedness) {
    auto program = BufferTests::MakeIntegerWaveKernel(waveSize, prefix, mask, signedness, true);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = waveSize;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = waveSize;
    info.hostSubgroupSize = 32;
    auto target = BufferTests::Target();
    std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    if (native) capabilities.push_back(spv::CapabilityGroupNonUniformArithmetic);
    target.supportedCapabilities = capabilities;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, target, {0, 0, 0, 128});
    unsigned operations = 0;
    unsigned shuffles = 0;
    for (std::size_t offset = 5; offset < shader.code.size(); offset += shader.code[offset] >> 16) {
        auto opcode = shader.code[offset] & 65535;
        if (opcode == spv::OpGroupNonUniformShuffle) ++shuffles;
        if (opcode == spv::OpGroupNonUniformIAdd) {
            ++operations;
            Fixtures::Require(shader.code[offset + 4] == static_cast<std::uint32_t>(prefix ? spv::GroupOperationExclusiveScan : spv::GroupOperationReduce));
        }
    }
    if (native && (operations != 1 || shuffles != 0)) throw std::runtime_error("Native integer wave operation was not reduced; prefix=" + std::to_string(prefix) + " operations=" + std::to_string(operations) + " shuffles=" + std::to_string(shuffles));
    if (!native && waveSize == 32 && mask == UINT32_MAX) CheckIntegerWaveProof(shader.code, prefix);
    std::array<std::uint32_t, 16> input{UINT32_MAX, 1, 0x80000000, 0x7FFFFFFF, 0xFFFF0001, 17, 0, 3, 0xAAAAAAAA, 0x55555555, 31, 32, 0x12345678, 0xDEADBEEF, 19, 7};
    std::vector<std::uint32_t> expected(waveSize * 2, UINT32_MAX);
    for (unsigned thread = 0; thread < expected.size(); ++thread) {
        auto lane = thread % waveSize;
        if (waveSize == 32 && (mask & (1u << lane)) == 0) continue;
        std::uint32_t value = 0;
        for (unsigned other = 0; other < (prefix ? lane : waveSize); ++other) {
            if (waveSize == 64 || (mask & (1u << other)) != 0) value += input[other % input.size()];
        }
        expected[thread] = value;
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1}, std::as_bytes(std::span(input)));
}

void CheckInvalid() {
    BufferTests::FloatFixture fixture;
    for (bool prefix : {false, true}) {
        auto* word = fixture.IntegerType(32);
        auto reject = [&](LLVMBC::Vector<LLVMBC::Value*> arguments) {
            fixture.Reject(*fixture.Call(prefix ? "dx.op.wavePrefixOp.i32" : "dx.op.waveActiveOp.i32", word, std::move(arguments)));
        };
        reject({});
        reject({fixture.Integer(32, prefix ? 119 : 121), fixture.Integer(32, 1), fixture.Integer(8, 0), fixture.Integer(8, 1)});
        reject({fixture.Integer(32, prefix ? 121 : 119), fixture.Integer(32, 1), fixture.Integer(8, 1), fixture.Integer(8, 1)});
        reject({fixture.Integer(32, prefix ? 121 : 119), fixture.Integer(32, 1), fixture.Integer(8, 0), fixture.Integer(8, 2)});
    }
}
}

void BufferTests::CheckWaveIntegerGpu(AnyBox::Vulkan::Context& context) {
    CheckInvalid();
    CheckWideWaveGpu(context);
    CheckWaveMinimum(context);
    for (bool prefix : {false, true}) {
        for (bool native : {false, true}) {
            for (auto mask : {UINT32_MAX, 0x80000000U, 0xFFFF0000U, 0xA55A8420U, 1U}) Check(context, 32, prefix, mask, native, 1);
            Check(context, 32, prefix, 0xA55A8420U, native, 0);
        }
        for (unsigned signedness : {0u, 1u}) Check(context, 64, prefix, UINT32_MAX, false, signedness);
    }
}
