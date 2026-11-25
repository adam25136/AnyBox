#include "Scalar.hpp"
#include "WaveSum.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::waveInteger(const LLVMBC::CallInst& instruction, bool prefix) {
    auto bits = width(instruction);
    if (instruction.getNumArgOperands() != 4 || (bits != 32 && bits != 64)) throw std::runtime_error("DXIL integer wave operation has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || width(*instruction.getArgOperand(1)) != bits || width(*instruction.getArgOperand(2)) != 8 || width(*instruction.getArgOperand(3)) != 8) throw std::runtime_error("DXIL integer wave operation has invalid operand types");
    auto constant = [&](unsigned index) {
        auto* value = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(index));
        if (!value) throw std::runtime_error("DXIL integer wave control operands must be constant");
        return value->getUniqueInteger().getZExtValue();
    };
    if (constant(0) != (prefix ? 121u : 119u) || constant(2) != 0 || constant(3) > 1) throw std::runtime_error("DXIL integer wave operation mode is not implemented");
    auto waveSize = program.WaveSize();
    if (waveSize != 32 && waveSize != 64) throw std::runtime_error("DXIL integer wave operation requires a 32-lane or 64-lane wave");

    auto& source = Get(*instruction.getArgOperand(1));
    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&builder.ConstantBool(true)});
    if (bits == 32) return SumWaveWords(builder, source, ballot, waveSize, prefix);
    if (auto* constantValue = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(1))) {
        auto& count = CountWaveLanes(builder, ballot, waveSize, prefix);
        auto& wideCount = builder.ConstructU64(count, builder.Constant(0));
        return builder.Emit(IrOpcode::IMul64, IrType::U64, {&wideCount, &builder.ConstantU64(constantValue->getUniqueInteger().getZExtValue())});
    }
    auto& low = builder.CompositeExtract(source, 0);
    auto& high = builder.CompositeExtract(source, 1);
    auto& lowerSum = SumWaveWords(builder, builder.BitwiseAnd(low, builder.Constant(65535)), ballot, waveSize, prefix);
    auto& upperSum = SumWaveWords(builder, builder.ShiftRightLogical(low, builder.Constant(16)), ballot, waveSize, prefix);
    auto& highSum = SumWaveWords(builder, high, ballot, waveSize, prefix);
    auto& middle = builder.IAdd(upperSum, builder.ShiftRightLogical(lowerSum, builder.Constant(16)));
    auto& resultLow = builder.BitwiseOr(builder.BitwiseAnd(lowerSum, builder.Constant(65535)), builder.ShiftLeftLogical(middle, builder.Constant(16)));
    auto& resultHigh = builder.IAdd(highSum, builder.ShiftRightLogical(middle, builder.Constant(16)));
    return builder.ConstructU64(resultLow, resultHigh);
}
