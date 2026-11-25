#include "BitCount.hpp"
#include "Scalar.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerMaskBitCount64(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program) {
    auto parts = ReadBitCountWords(instruction, scalar, builder, 107);
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("Xbox masked bit count requires a 32-lane or 64-lane wave");
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& local = builder.BitwiseAnd(lane, builder.Constant(31));
    auto& below = builder.ISub(builder.ShiftLeftLogical(builder.Constant(1), local), builder.Constant(1));
    if (program.WaveSize() == 32) return builder.Emit(IrOpcode::BitCount32, IrType::U32, {&builder.BitwiseAnd(*parts[0], below)});
    auto& upper = builder.Emit(IrOpcode::UGreaterThanEqual32, IrType::Bool, {&lane, &builder.Constant(32)});
    auto& lowMask = builder.Select(upper, builder.Constant(UINT32_MAX), below);
    auto& highMask = builder.Select(upper, below, builder.Constant(0));
    auto& low = builder.Emit(IrOpcode::BitCount32, IrType::U32, {&builder.BitwiseAnd(*parts[0], lowMask)});
    auto& high = builder.Emit(IrOpcode::BitCount32, IrType::U32, {&builder.BitwiseAnd(*parts[1], highMask)});
    return builder.IAdd(low, high);
}
