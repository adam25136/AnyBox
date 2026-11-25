#include "UnaryInteger.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerUnaryInteger(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, unsigned bits, bool reverse) {
    if ((bits != 32 && bits != 64) || (!reverse && bits != 64)) throw std::invalid_argument("DXIL unary integer overload is unsupported");
    auto integer = [](const LLVMBC::Value& value, unsigned width) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == width; };
    if (instruction.getNumArgOperands() != 2 || !integer(instruction, reverse ? bits : 32) || !integer(*instruction.getArgOperand(0), 32) || !integer(*instruction.getArgOperand(1), bits)) throw std::runtime_error("DXIL unary integer operation has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode) throw std::runtime_error("DXIL unary integer operation requires a constant opcode");
    auto operation = opcode->getUniqueInteger().getZExtValue();
    if (reverse ? operation != 30 : operation < 31 || operation > 34) throw std::runtime_error("DXIL unary integer operation opcode is unsupported");
    auto& value = scalar.Get(*instruction.getArgOperand(1));
    if (bits == 32) return builder.Emit(IrOpcode::BitReverse32, IrType::U32, {&value});
    if (operation == 31) return builder.Emit(IrOpcode::BitCount64, IrType::U32, {&value});
    auto* low = &builder.CompositeExtract(value, 0);
    auto* high = &builder.CompositeExtract(value, 1);
    if (reverse) {
        auto& reverseLow = builder.Emit(IrOpcode::BitReverse32, IrType::U32, {low});
        auto& reverseHigh = builder.Emit(IrOpcode::BitReverse32, IrType::U32, {high});
        return builder.ConstructU64(reverseHigh, reverseLow);
    }
    if (operation == 32) {
        auto& lowBit = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {low});
        auto& highBit = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {high});
        auto& upper = builder.Select(builder.IEqual(*high, builder.Constant(0)), builder.Constant(UINT32_MAX), builder.IAdd(highBit, builder.Constant(32)));
        return builder.Select(builder.IEqual(*low, builder.Constant(0)), upper, lowBit);
    }
    if (operation == 34) {
        auto& negative = builder.INotEqual(builder.BitwiseAnd(*high, builder.Constant(0x80000000)), builder.Constant(0));
        low = &builder.Select(negative, builder.BitwiseNot(*low), *low);
        high = &builder.Select(negative, builder.BitwiseNot(*high), *high);
    }
    auto& normalized = builder.ConstructU64(*low, *high);
    auto& highest = builder.Emit(IrOpcode::FindUMsb64, IrType::U32, {&normalized});
    auto& leading = builder.ISub(builder.Constant(63), highest);
    return builder.Select(builder.IEqual(builder.BitwiseOr(*low, *high), builder.Constant(0)), builder.Constant(UINT32_MAX), leading);
}
