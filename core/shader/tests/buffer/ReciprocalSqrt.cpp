#include "FloatBitsKernel.hpp"
#include "FloatFixture.hpp"
#include "GpuValues.hpp"
#include "Fixtures.hpp"
#include <spirv/unified1/GLSL.std.450.h>
#include <bit>
#include <cmath>

namespace {
void CheckResult(std::uint32_t input, std::uint32_t output) {
    auto magnitude = input & 0x7FFFFFFFU;
    if (magnitude < 0x00800000U) {
        Fixtures::Require(output == ((input & 0x80000000U) | 0x7F800000U));
    } else if (input == 0x7F800000U) {
        Fixtures::Require(output == 0);
    } else if ((input & 0x80000000U) != 0 || magnitude > 0x7F800000U) {
        Fixtures::Require(std::isnan(std::bit_cast<float>(output)));
    } else {
        auto expected = 1.0 / std::sqrt(static_cast<double>(std::bit_cast<float>(input)));
        auto observed = static_cast<double>(std::bit_cast<float>(output));
        if (!std::isfinite(observed) || std::abs(observed / expected - 1.0) > 0x1p-21) throw std::runtime_error("Reciprocal square root exceeded the Direct3D relative error bound for " + std::to_string(input));
    }
}

void CheckNative(std::span<const std::uint32_t> code) {
    unsigned inverseRoots = 0;
    for (std::size_t offset = 5; offset < code.size();) {
        auto words = code[offset] >> 16;
        auto opcode = code[offset] & 65535;
        Fixtures::Require(words != 0 && words <= code.size() - offset);
        Fixtures::Require(opcode != spv::OpFDiv);
        if (opcode == spv::OpExtInst && words >= 5) {
            Fixtures::Require(code[offset + 4] != GLSLstd450Sqrt);
            if (code[offset + 4] == GLSLstd450InverseSqrt) ++inverseRoots;
        }
        offset += words;
    }
    Fixtures::Require(inverseRoots == 1);
}

void CheckConstants(AnyBox::Vulkan::Context& context) {
    BufferTests::FloatFixture fixture;
    auto* opcode = fixture.Integer(32, 25);
    std::array input{0u, 0x80000000u, 1u, 0x80000001u, 0x7F800000u, 0xFF800000u, 0x7FC12345u, 0xBF800000u, 0x3F800000u, 0x40800000u, 0x3E800000u, 0x00800000u, 0x7E800000u};
    std::array expected{0x7F800000u, 0xFF800000u, 0x7F800000u, 0xFF800000u, 0u, 0x7FC00000u, 0x7FC00000u, 0x7FC00000u, 0x3F800000u, 0x3F000000u, 0x40000000u, 0x5F000000u, 0x20000000u};
    for (std::size_t index = 0; index < input.size(); ++index) {
        auto* call = fixture.Call("dx.op.unary.f32", fixture.FloatType(), {opcode, fixture.FloatConstant(input[index])});
        fixture.Expect(*call, expected[index]);
    }
    std::array<std::uint32_t, 16> unused{};
    fixture.Run(context, unused);
}
}

void BufferTests::CheckReciprocalSqrt(AnyBox::Vulkan::Context& context) {
    auto program = MakeFloatBitsKernel(25, true);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
    CheckNative(shader.code);
    std::vector<std::uint32_t> input{0, 0x80000000, 1, 0x007FFFFF, 0x80000001, 0x807FFFFF, 0x7F800000, 0xFF800000, 0x7FC12345, 0xFF812345, 0x7F800001, 0xFFBFFFFF};
    for (unsigned exponent = 1; exponent < 255; ++exponent) {
        for (unsigned mantissa : {0u, 1u, 0x3FFFFFu, 0x400000u, 0x7FFFFEu, 0x7FFFFFu}) {
            auto bits = (exponent << 23) | mantissa;
            input.push_back(bits);
            input.push_back(bits | 0x80000000U);
        }
    }
    std::uint32_t random = 0x254DE018;
    for (unsigned index = 0; index < 65536; ++index) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        input.push_back(random);
    }
    while (input.size() % 64 != 0) input.push_back(0x3F800000);
    auto values = ReadGpu(context, shader, input.size(), {static_cast<std::uint32_t>(input.size() / 64), 1, 1}, std::as_bytes(std::span(input)));
    for (std::size_t index = 0; index < input.size(); ++index) CheckResult(input[index], values[index]);
    CheckConstants(context);
}
