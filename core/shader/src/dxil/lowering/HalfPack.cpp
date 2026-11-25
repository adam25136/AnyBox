#include "HalfPack.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerFloatToHalf(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 2 || !word(instruction) || !word(*instruction.getArgOperand(0)) || ScalarLowering::GetType(*instruction.getArgOperand(1)) != IrType::F32) throw std::runtime_error("DXIL float-to-half conversion has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 130) throw std::runtime_error("DXIL float-to-half conversion opcode is invalid");
    return builder.Emit(IrOpcode::PackFloat2x16Rtz, IrType::U32, {&scalar.Get(*instruction.getArgOperand(1)), &builder.ConstantF32(0)});
}
