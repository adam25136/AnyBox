#include "BitCount.hpp"
#include "Scalar.hpp"
#include "vector/Words.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerBitCount64(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 2 || !word(instruction) || !word(*instruction.getArgOperand(0))) throw std::runtime_error("Xbox 64-bit bit count has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 130) throw std::runtime_error("Xbox 64-bit bit count opcode is invalid");
    auto parts = ReadWordPair(*instruction.getArgOperand(1), scalar, builder);
    auto& value = builder.ConstructU64(*parts[0], *parts[1]);
    return builder.Emit(IrOpcode::BitCount64, IrType::U32, {&value});
}
