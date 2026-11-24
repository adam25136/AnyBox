#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::waveFirst(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 1 || GetType(instruction) != IrType::Bool) throw std::runtime_error("DXIL first wave lane intrinsic has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32) throw std::runtime_error("DXIL first wave lane intrinsic requires a 32-bit opcode");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 110) throw std::runtime_error("DXIL first wave lane intrinsic opcode must be constant 110");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("DXIL first wave lane intrinsic requires a 32-lane or 64-lane wave");

    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&builder.ConstantBool(true)});
    auto& low = builder.CompositeExtract(ballot, 0);
    auto& first = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {&low});
    if (program.WaveSize() == 64) {
        auto& high = builder.CompositeExtract(ballot, 1);
        auto& upperFirst = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {&high});
        auto& fullFirst = builder.Select(builder.IEqual(low, builder.Constant(0)), builder.IAdd(upperFirst, builder.Constant(32)), first);
        return builder.IEqual(lane, fullFirst);
    }
    return builder.IEqual(lane, first);
}
