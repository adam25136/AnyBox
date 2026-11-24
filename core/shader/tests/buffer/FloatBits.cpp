#include "FloatFixture.hpp"
#include "FloatBitsKernel.hpp"
#include <bit>
#include <cmath>

namespace {
std::uint32_t Fraction(std::uint32_t bits) {
    if ((bits & 0x7FFFFFFFU) >= 0x7F800000U) return 0x7FC00000;
    if ((bits & 0x7F800000U) == 0) bits &= 0x80000000U;
    auto value = std::bit_cast<float>(bits);
    auto fraction = value - std::floor(value);
    if (fraction >= 1) return 0x3F7FFFFF;
    return std::bit_cast<std::uint32_t>(fraction);
}

std::uint32_t RoundedOrSaturated(std::uint32_t bits, unsigned mode) {
    if ((bits & 0x7FFFFFFFU) > 0x7F800000U) return mode == 7 ? 0 : 0x7FC00000;
    if ((bits & 0x7F800000U) == 0) bits &= 0x80000000U;
    auto value = std::bit_cast<float>(bits);
    if (mode == 28) return std::bit_cast<std::uint32_t>(std::ceil(value));
    if (value <= 0) return 0;
    if (value >= 1) return 0x3F800000;
    return bits;
}

void CheckInvalid() {
    BufferTests::FloatFixture fixture;
    auto* floating = fixture.FloatType();
    auto* value = fixture.Argument(0);
    auto* opcode = fixture.Integer(32, 6);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) { fixture.Reject(*fixture.Call("dx.op.unary.f32", result, std::move(arguments))); };
    reject(floating, {});
    reject(floating, {opcode});
    reject(floating, {opcode, value, value});
    reject(fixture.IntegerType(32), {opcode, value});
    reject(floating, {fixture.Integer(16, 6), value});
    reject(floating, {value, value});
    reject(floating, {fixture.Integer(32, 8), value});
    reject(floating, {opcode, fixture.Integer(32, 0)});
    fixture.Reject(*fixture.Call("llvm.copysign.f32", floating, {}));
    fixture.Reject(*fixture.Call("llvm.copysign.f32", floating, {value}));
    fixture.Reject(*fixture.Call("llvm.copysign.f32", floating, {value, value, value}));
    fixture.Reject(*fixture.Call("llvm.copysign.f32", fixture.IntegerType(32), {value, value}));
    fixture.Reject(*fixture.Call("llvm.copysign.f32", floating, {fixture.Integer(32, 0), value}));
    fixture.Reject(*fixture.Call("llvm.copysign.f32", floating, {value, fixture.Integer(32, 0)}));
}
}

namespace BufferTests {
void CheckFloatBits(AnyBox::Vulkan::Context& context) {
    CheckInvalid();
    std::array<std::uint32_t, 16> input{0, 0x80000000, 0x3F800000, 0xBF800000, 0x3F000000, 0xBF000000, 0x3EAAAAAB, 0xBEAAAAAB, 0x7FC12345, 0xFF812345, 0x7F800000, 0xFF800000, 1, 0x80000001, 0x00800000, 0x80800000};
    for (unsigned mode : {0u, 6u, 7u, 22u, 28u}) {
        auto program = MakeFloatBitsKernel(mode);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 64;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (auto value : input) {
            for (auto sign : input) {
                auto bits = (value & 0x7FFFFFFFU) | (mode == 0 ? sign & 0x80000000U : 0);
                if (mode == 22) bits = Fraction(value);
                if (mode == 7 || mode == 28) bits = RoundedOrSaturated(value, mode);
                expected.push_back(bits);
            }
        }
        RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(input)));
    }
}
}
