#include "BitSelect.hpp"
#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerBitSelect(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 4 || !word(instruction)) throw std::runtime_error("Xbox bit select has an invalid signature");
    for (unsigned index = 0; index < 4; ++index) {
        if (!word(*instruction.getArgOperand(index))) throw std::runtime_error("Xbox bit select requires 32-bit integers");
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 23) throw std::runtime_error("Xbox bit select opcode does not match its declaration");

    auto& mask = scalar.Get(*instruction.getArgOperand(1));
    auto& selected = scalar.Get(*instruction.getArgOperand(2));
    auto& base = scalar.Get(*instruction.getArgOperand(3));
    return builder.BitwiseOr(builder.BitwiseAnd(mask, selected), builder.BitwiseAnd(builder.BitwiseNot(mask), base));
}
