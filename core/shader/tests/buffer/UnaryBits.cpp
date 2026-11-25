#include "FloatFixture.hpp"
#include <bit>

void CheckWideUnaryGpu(AnyBox::Vulkan::Context& context);

namespace {
std::uint32_t Expected(unsigned opcode, std::uint32_t value) {
    if (opcode == 30) {
        std::uint32_t result = 0;
        for (unsigned index = 0; index < 32; ++index) result |= ((value >> index) & 1) << (31 - index);
        return result;
    }
    if (opcode == 31) return static_cast<std::uint32_t>(std::popcount(value));
    if (opcode == 32) return value == 0 ? UINT32_MAX : static_cast<std::uint32_t>(std::countr_zero(value));
    if (opcode == 34 && (value & 0x80000000U) != 0) value = ~value;
    return value == 0 ? UINT32_MAX : static_cast<std::uint32_t>(std::countl_zero(value));
}

void CheckBatch(AnyBox::Vulkan::Context& context, const std::array<std::uint32_t, 16>& input) {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    for (unsigned index = 0; index < input.size(); ++index) {
        auto* value = fixture.Make<LLVMBC::CastInst>(word, fixture.Argument(index), LLVMBC::Instruction::BitCast);
        for (unsigned opcode = 30; opcode <= 34; ++opcode) {
            auto* name = opcode == 30 ? "dx.op.unary.i32" : "dx.op.unaryBits.i32";
            auto* call = fixture.Call(name, word, {fixture.Integer(32, opcode), value});
            fixture.Expect(*call, Expected(opcode, input[index]));
            auto* constant = fixture.Call(name, word, {fixture.Integer(32, opcode), fixture.Integer(32, input[index])});
            fixture.Expect(*constant, Expected(opcode, input[index]));
        }
    }
    fixture.Run(context, input);
}

void CheckInvalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        fixture.Reject(*fixture.Call("dx.op.unaryBits.i32", result, std::move(arguments)));
    };
    reject(word, {});
    reject(word, {fixture.Integer(32, 31)});
    reject(word, {fixture.Integer(32, 31), fixture.Integer(64, 1)});
    reject(word, {fixture.Integer(16, 31), fixture.Integer(32, 1)});
    reject(word, {fixture.Integer(32, 29), fixture.Integer(32, 1)});
    reject(word, {fixture.Integer(32, 30), fixture.Integer(32, 1)});
    reject(word, {fixture.Integer(32, 35), fixture.Integer(32, 1)});
    reject(fixture.IntegerType(64), {fixture.Integer(32, 31), fixture.Integer(32, 1)});
    auto* dynamic = fixture.Make<LLVMBC::CastInst>(word, fixture.Argument(0), LLVMBC::Instruction::BitCast);
    reject(word, {dynamic, fixture.Integer(32, 1)});
}
}

void BufferTests::CheckUnaryBitsGpu(AnyBox::Vulkan::Context& context) {
    CheckInvalid();
    CheckWideUnaryGpu(context);
    std::array<std::uint32_t, 16> input{0, 1, UINT32_MAX, 0x80000000, 0x7FFFFFFF, 0xFFFFFFFE, 0xAAAAAAAA, 0x55555555, 0x12345678, 0xDEADBEEF, 16, 31, 32, 33, 63, 64};
    CheckBatch(context, input);
    for (unsigned batch = 0; batch < 4; ++batch) {
        for (unsigned index = 0; index < input.size(); ++index) {
            auto bit = (std::uint32_t{1} << ((batch & 1) * 16 + index));
            input[index] = batch < 2 ? bit : ~bit;
        }
        CheckBatch(context, input);
    }
}
