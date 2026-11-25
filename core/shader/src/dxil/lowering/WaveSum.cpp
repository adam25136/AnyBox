#include "WaveSum.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::SumWaveWords(IrBuilder& builder, IrValue& source, IrValue& ballot, unsigned waveSize, bool prefix) {
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

IrValue& AnyBox::Shader::Dxil::CountWaveLanes(IrBuilder& builder, IrValue& ballot, unsigned waveSize, bool prefix) {
    auto* low = &builder.CompositeExtract(ballot, 0);
    auto* high = &builder.CompositeExtract(ballot, 1);
    if (prefix) {
        auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
        auto& below = builder.ISub(builder.ShiftLeftLogical(builder.Constant(1), builder.BitwiseAnd(lane, builder.Constant(31))), builder.Constant(1));
        auto& upper = builder.Emit(IrOpcode::UGreaterThanEqual32, IrType::Bool, {&lane, &builder.Constant(32)});
        low = &builder.BitwiseAnd(*low, waveSize == 32 ? below : builder.Select(upper, builder.Constant(UINT32_MAX), below));
        high = &builder.BitwiseAnd(*high, builder.Select(upper, below, builder.Constant(0)));
    }
    auto& count = builder.Emit(IrOpcode::BitCount32, IrType::U32, {low});
    if (waveSize == 32) return count;
    return builder.IAdd(count, builder.Emit(IrOpcode::BitCount32, IrType::U32, {high}));
}
