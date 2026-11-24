#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::waveInteger(const LLVMBC::CallInst& instruction, bool prefix) {
    if (instruction.getNumArgOperands() != 4 || width(instruction) != 32) throw std::runtime_error("DXIL integer wave operation has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || width(*instruction.getArgOperand(1)) != 32 || width(*instruction.getArgOperand(2)) != 8 || width(*instruction.getArgOperand(3)) != 8) throw std::runtime_error("DXIL integer wave operation has invalid operand types");
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
    auto& low = builder.CompositeExtract(ballot, 0);
    auto& high = builder.CompositeExtract(ballot, 1);
    auto& lowFirst = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {&low});
    IrValue* first = &lowFirst;
    if (waveSize == 64) {
        auto& highFirst = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {&high});
        first = &builder.Select(builder.IEqual(low, builder.Constant(0)), builder.IAdd(highFirst, builder.Constant(32)), lowFirst);
    }
    auto& ownLane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    IrValue* result = &builder.Constant(0);
    for (unsigned lane = 0; lane < waveSize; ++lane) {
        auto& word = lane < 32 ? low : high;
        auto& bit = builder.BitwiseAnd(word, builder.Constant(std::uint32_t{1} << (lane & 31)));
        auto& present = builder.INotEqual(bit, builder.Constant(0));
        auto& index = builder.Select(present, builder.Constant(lane), *first);
        auto& value = builder.Emit(IrOpcode::ReadLane, IrType::U32, {&source, &index});
        auto& included = prefix ? builder.LogicalAnd(present, builder.ULessThan(builder.Constant(lane), ownLane)) : present;
        auto& term = builder.Select(included, value, builder.Constant(0));
        result = &builder.IAdd(*result, term);
    }
    return *result;
}
