#include "HalfExtrema.hpp"
#include "HalfScalar.hpp"
#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader::Dxil;

IrValue& AnyBox::Shader::Dxil::HalfExtremumBits(IrBuilder& builder, IrValue& left, IrValue& right, bool maximum) {
    auto& a = builder.BitwiseAnd(left, builder.Constant(0x7FFF));
    auto& b = builder.BitwiseAnd(right, builder.Constant(0x7FFF));
    auto key = [&](IrValue& value) -> IrValue& {
        auto& negative = builder.INotEqual(builder.BitwiseAnd(value, builder.Constant(0x8000)), builder.Constant(0));
        auto& flip = builder.Select(negative, builder.Constant(0xFFFF), builder.Constant(0x8000));
        return builder.BitwiseXor(value, flip);
    };
    auto& firstKey = key(left);
    auto& secondKey = key(right);
    auto& first = maximum ? builder.UGreaterThan(firstKey, secondKey) : builder.ULessThan(firstKey, secondKey);
    auto& numeric = builder.Select(first, left, right);
    auto& nanA = builder.UGreaterThan(a, builder.Constant(0x7C00));
    auto& nanB = builder.UGreaterThan(b, builder.Constant(0x7C00));
    return builder.Select(nanB, left, builder.Select(nanA, right, numeric));
}

IrValue& AnyBox::Shader::Dxil::LowerHalfExtremum(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder) {
    if (instruction.getNumArgOperands() != 3 || !IsHalf(instruction) || !IsHalf(*instruction.getArgOperand(1)) || !IsHalf(*instruction.getArgOperand(2))) throw std::runtime_error("DXIL half min/max has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || !opcode->getType()->isIntegerTy() || opcode->getType()->getIntegerBitWidth() != 32) throw std::runtime_error("DXIL half min/max requires a constant 32-bit opcode");
    auto operation = opcode->getUniqueInteger().getZExtValue();
    if (operation != 35 && operation != 36) throw std::runtime_error("DXIL half binary opcode is not implemented");
    auto& left = builder.BitCastU32FromF16(scalar.Get(*instruction.getArgOperand(1)));
    auto& right = builder.BitCastU32FromF16(scalar.Get(*instruction.getArgOperand(2)));
    return builder.BitCastF16(HalfExtremumBits(builder, left, right, operation == 35));
}
