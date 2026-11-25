#include "ReadLane.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerReadLane(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 3 || !word(instruction)) throw std::runtime_error("Xbox read lane intrinsic has an invalid signature");
    for (unsigned index = 0; index < 3; ++index) if (!word(*instruction.getArgOperand(index))) throw std::runtime_error("Xbox read lane intrinsic requires 32-bit integer operands");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 51) throw std::runtime_error("Xbox read lane intrinsic opcode is invalid");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("Xbox read lane requires a 32-lane or 64-lane wave");
    auto& value = scalar.Get(*instruction.getArgOperand(1));
    auto& selector = scalar.Get(*instruction.getArgOperand(2));
    return builder.Emit(IrOpcode::ReadLane, IrType::U32, {&value, &selector});
}
