#include "HalfCastKernel.hpp"
#include "HalfCastReference.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

void CheckHalfCastInputs(AnyBox::Vulkan::Context& context);

namespace {
void Check(AnyBox::Vulkan::Context& context, bool narrow, std::span<const std::uint32_t> input) {
    auto program = BufferTests::MakeHalfCastKernel(narrow);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    for (std::size_t offset = 5; offset < shader.code.size(); offset += shader.code[offset] >> 16) {
        auto opcode = shader.code[offset] & 65535;
        if (opcode == spv::OpTypeFloat || opcode == spv::OpTypeInt) Fixtures::Require(shader.code[offset + 2] != 16);
    }
    std::vector<std::uint32_t> expected;
    for (std::size_t index = 0; index < input.size(); ++index) {
        auto bits = narrow ? BufferTests::RoundHalfReference(input[index]) : input[index] & 65535;
        auto chosen = bits ^ (index % 2 ? 0x8000U : 0U);
        expected.push_back(bits);
        expected.push_back(BufferTests::WidenCanonicalHalf(bits));
        expected.push_back(narrow ? BufferTests::CanonicalHalf(chosen) : chosen);
    }
    Fixtures::Require(input.size() % 64 == 0);
    BufferTests::RunGpu(context, shader, expected, {static_cast<std::uint32_t>(input.size() / 64), 1, 1}, std::as_bytes(input));
}
}

void CheckHalfCasts(AnyBox::Vulkan::Context& context) {
    CheckHalfCastInputs(context);
    std::vector<std::uint32_t> input;
    for (unsigned half = 0; half < 65536; ++half) input.push_back(0xDEAD0000U | half);
    Check(context, false, input);
    input.clear();
    for (unsigned half = 0; half < 65536; ++half) {
        auto bits = BufferTests::WidenHalfReference(half);
        input.push_back(bits);
        input.push_back(bits + 1);
        input.push_back(bits - 1);
    }
    for (unsigned exponent = 0; exponent < 256; ++exponent) {
        for (auto mantissa : {0u, 1u, 4095u, 4096u, 4097u, 8191u, 8192u, 8193u, 0x3FFFFFu, 0x400000u, 0x7FDFFFu, 0x7FE000u, 0x7FEFFFu, 0x7FF000u, 0x7FFFFEu, 0x7FFFFFu}) {
            for (auto sign : {0u, 0x80000000u}) input.push_back(sign | (exponent << 23) | mantissa);
        }
    }
    std::uint32_t random = 0x49D10BCE;
    for (unsigned index = 0; index < 65536; ++index) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        input.push_back(random);
    }
    Check(context, true, input);
}
