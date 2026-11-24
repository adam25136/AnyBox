#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::unaryBits(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 2 || width(instruction) != 32) throw std::runtime_error("DXIL unary bit operation has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || width(*instruction.getArgOperand(1)) != 32) throw std::runtime_error("DXIL unary bit operation requires 32-bit operands");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode) throw std::runtime_error("DXIL unary bit operation requires a constant opcode");
    auto operation = opcode->getUniqueInteger().getZExtValue();
    if (operation < 30 || operation > 34) throw std::runtime_error("DXIL unary bit operation is not implemented: " + std::to_string(operation));
    auto& value = Get(*instruction.getArgOperand(1));
    if (operation == 30) return builder.Emit(IrOpcode::BitReverse32, IrType::U32, {&value});
    if (operation == 31) return builder.Emit(IrOpcode::BitCount32, IrType::U32, {&value});
    if (operation == 32) return builder.Emit(IrOpcode::FindILsb32, IrType::U32, {&value});

    IrValue* normalized = &value;
    if (operation == 34) {
        auto& negative = builder.INotEqual(builder.BitwiseAnd(value, builder.Constant(0x80000000U)), builder.Constant(0));
        normalized = &builder.Select(negative, builder.BitwiseNot(value), value);
    }
    auto& highest = builder.Emit(IrOpcode::FindUMsb32, IrType::U32, {normalized});
    auto& leading = builder.ISub(builder.Constant(31), highest);
    return builder.Select(builder.IEqual(*normalized, builder.Constant(0)), builder.Constant(UINT32_MAX), leading);
}
