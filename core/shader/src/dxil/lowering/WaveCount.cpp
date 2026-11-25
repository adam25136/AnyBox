#include "Scalar.hpp"
#include "WaveCount.hpp"
#include "WaveSum.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& AnyBox::Shader::Dxil::LowerWaveBitCount(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program, bool prefix) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 2 || !word(instruction)) throw std::runtime_error("DXIL wave bit count has an invalid signature");
    if (!word(*instruction.getArgOperand(0)) || ScalarLowering::GetType(*instruction.getArgOperand(1)) != IrType::Bool) throw std::runtime_error("DXIL wave bit count requires a 32-bit opcode and boolean predicate");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != (prefix ? 136u : 135u)) throw std::runtime_error("DXIL wave bit count opcode does not match its declaration");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("DXIL wave bit count requires a 32-lane or 64-lane wave");

    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&scalar.Get(*instruction.getArgOperand(1))});
    return CountWaveLanes(builder, ballot, program.WaveSize(), prefix);
}

IrValue& ScalarLowering::waveCount(const LLVMBC::CallInst& instruction) { return LowerWaveBitCount(instruction, *this, builder, program, false); }
