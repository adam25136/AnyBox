#include "FloatFixture.hpp"
#include "HalfCastReference.hpp"

void CheckHalfCastInputs(AnyBox::Vulkan::Context& context) {
    BufferTests::FloatFixture fixture;
    auto* floating = fixture.FloatType();
    auto* half = LLVMBC::Type::getHalfTy(floating->getContext());
    auto* integer = fixture.IntegerType(16);
    auto* zero = LLVMBC::ConstantFP::get(half, 0);
    auto reject = [&](LLVMBC::Type* target, LLVMBC::Value* source, LLVMBC::Instruction::CastOps operation) { fixture.Reject(*fixture.Make<LLVMBC::CastInst>(target, source, operation)); };
    reject(half, fixture.FloatConstant(0), LLVMBC::Instruction::FPExt);
    reject(floating, zero, LLVMBC::Instruction::FPTrunc);
    reject(half, fixture.Integer(16, 0), LLVMBC::Instruction::FPTrunc);
    reject(integer, zero, LLVMBC::Instruction::FPExt);
    reject(half, fixture.Integer(32, 0), LLVMBC::Instruction::BitCast);
    reject(fixture.IntegerType(8), zero, LLVMBC::Instruction::BitCast);
    reject(half, fixture.Integer(16, 1), LLVMBC::Instruction::UIToFP);
    for (unsigned bits : {0u, 0x8000u, 1u, 0x8001u, 0x03FFu, 0x0400u, 0x3C00u, 0x7BFFu, 0x7C00u, 0xFC00u, 0x7C01u, 0x7E43u, 0xFD45u}) {
        auto* value = LLVMBC::ConstantFP::get(half, bits);
        fixture.Expect(*fixture.Make<LLVMBC::CastInst>(integer, value, LLVMBC::Instruction::BitCast), bits);
        auto* selected = fixture.Make<LLVMBC::SelectInst>(value, zero, fixture.Integer(1, 1));
        fixture.Expect(*fixture.Make<LLVMBC::CastInst>(integer, selected, LLVMBC::Instruction::BitCast), bits);
        auto* negated = fixture.Make<LLVMBC::UnaryOperator>(LLVMBC::UnaryOperator::UnaryOps::FNeg, value);
        fixture.Expect(*fixture.Make<LLVMBC::CastInst>(integer, negated, LLVMBC::Instruction::BitCast), bits ^ 0x8000);
    }
    for (auto* value : {static_cast<LLVMBC::Value*>(LLVMBC::UndefValue::get(half)), static_cast<LLVMBC::Value*>(zero)}) fixture.Expect(*fixture.Make<LLVMBC::CastInst>(integer, value, LLVMBC::Instruction::BitCast), 0);
    std::array<std::uint32_t, 16> input{0, 0x80000000, 0x3F801000, 0x3F803000, 0xBF801000, 0x477FE000, 0x477FF000, 0x477FEFFF, 0x33800000, 0x33000000, 0x33000001, 0x387FFFFF, 0x38800000, 0x7F7FFFFF, 0x7F800000, 0xFF800000};
    for (unsigned index = 0; index < input.size(); ++index) {
        for (auto* source : {fixture.Argument(index), static_cast<LLVMBC::Value*>(fixture.FloatConstant(input[index]))}) {
            auto* narrowed = fixture.Make<LLVMBC::CastInst>(half, source, LLVMBC::Instruction::FPTrunc);
            fixture.Expect(*fixture.Make<LLVMBC::CastInst>(integer, narrowed, LLVMBC::Instruction::BitCast), BufferTests::RoundHalfReference(input[index]));
        }
    }
    fixture.Run(context, input);
}
