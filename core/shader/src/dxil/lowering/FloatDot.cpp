#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::floatDot(const LLVMBC::CallInst& instruction, unsigned dimensions) {
    if (dimensions < 2 || dimensions > 4 || instruction.getNumArgOperands() != 1 + dimensions * 2 || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL dot product has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || width(*opcode) != 32 || opcode->getUniqueInteger().getZExtValue() != 52 + dimensions) throw std::runtime_error("DXIL dot product opcode does not match its declaration");
    for (unsigned index = 1; index < instruction.getNumArgOperands(); ++index) {
        if (GetType(*instruction.getArgOperand(index)) != IrType::F32) throw std::runtime_error("DXIL dot product requires float32 operands");
    }

    auto& left = Get(*instruction.getArgOperand(1));
    auto& right = Get(*instruction.getArgOperand(1 + dimensions));
    auto* result = &builder.Emit(IrOpcode::FPMul32, IrType::F32, {&left, &right});
    bool precise = instruction.hasMetadata("dx.precise");
    for (unsigned index = 1; index < dimensions; ++index) {
        auto& a = Get(*instruction.getArgOperand(1 + index));
        auto& b = Get(*instruction.getArgOperand(1 + dimensions + index));
        if (precise) {
            auto& product = builder.Emit(IrOpcode::FPMul32, IrType::F32, {&a, &b});
            result = &builder.Emit(IrOpcode::FPAdd32, IrType::F32, {result, &product});
        } else result = &builder.Emit(IrOpcode::FPFma32, IrType::F32, {&a, &b, result});
    }
    return *result;
}
