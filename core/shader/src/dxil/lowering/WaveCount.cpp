#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::waveCount(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 2 || width(instruction) != 32) throw std::runtime_error("DXIL wave bit count has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || GetType(*instruction.getArgOperand(1)) != IrType::Bool) throw std::runtime_error("DXIL wave bit count requires a 32-bit opcode and boolean predicate");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 135) throw std::runtime_error("DXIL wave bit count opcode must be constant 135");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("DXIL wave bit count requires a 32-lane or 64-lane wave");

    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&Get(*instruction.getArgOperand(1))});
    auto& low = builder.CompositeExtract(ballot, 0);
    auto& count = builder.Emit(IrOpcode::BitCount32, IrType::U32, {&low});
    if (program.WaveSize() == 32) return count;
    auto& high = builder.CompositeExtract(ballot, 1);
    auto& upperCount = builder.Emit(IrOpcode::BitCount32, IrType::U32, {&high});
    return builder.IAdd(count, upperCount);
}
