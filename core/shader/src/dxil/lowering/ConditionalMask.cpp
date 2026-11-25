#include "ConditionalMask.hpp"
#include "Scalar.hpp"
#include "vector/Words.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerConditionalMask(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 3 || !word(instruction) || !word(*instruction.getArgOperand(0))) throw std::runtime_error("Xbox conditional mask has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 170) throw std::runtime_error("Xbox conditional mask opcode is invalid");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("Xbox conditional mask requires a 32-lane or 64-lane wave");
    auto mask = ReadWordPair(*instruction.getArgOperand(1), scalar, builder);
    auto values = ReadWordPair(*instruction.getArgOperand(2), scalar, builder);
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& halfMask = program.WaveSize() == 64 ? builder.Select(builder.ULessThan(lane, builder.Constant(32)), *mask[0], *mask[1]) : *mask[0];
    auto& shift = builder.BitwiseAnd(lane, builder.Constant(31));
    auto& bit = builder.BitwiseAnd(builder.ShiftRightLogical(halfMask, shift), builder.Constant(1));
    auto& condition = builder.INotEqual(bit, builder.Constant(0));
    return builder.Select(condition, *values[1], *values[0]);
}
