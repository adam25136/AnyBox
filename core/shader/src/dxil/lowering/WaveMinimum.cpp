#include "WaveMinimum.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerWaveMinimum(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 2 || !word(instruction) || !word(*instruction.getArgOperand(0)) || !word(*instruction.getArgOperand(1))) throw std::runtime_error("Xbox unsigned wave minimum has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 113) throw std::runtime_error("Xbox unsigned wave minimum opcode is invalid");
    auto waveSize = program.WaveSize();
    if (waveSize != 32 && waveSize != 64) throw std::runtime_error("Xbox unsigned wave minimum requires a 32-lane or 64-lane wave");
    auto& source = scalar.Get(*instruction.getArgOperand(1));
    if (source.HasImmediate()) return source;

    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&builder.ConstantBool(true)});
    auto& low = builder.CompositeExtract(ballot, 0);
    auto& high = builder.CompositeExtract(ballot, 1);
    auto& lowFirst = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {&low});
    IrValue* first = &lowFirst;
    if (waveSize == 64) {
        auto& highFirst = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {&high});
        first = &builder.Select(builder.IEqual(low, builder.Constant(0)), builder.IAdd(highFirst, builder.Constant(32)), lowFirst);
    }
    IrValue* result = &source;
    for (unsigned lane = 0; lane < waveSize; ++lane) {
        auto& active = lane < 32 ? low : high;
        auto& bit = builder.BitwiseAnd(active, builder.Constant(std::uint32_t{1} << (lane & 31)));
        auto& present = builder.INotEqual(bit, builder.Constant(0));
        auto& selected = builder.Select(present, builder.Constant(lane), *first);
        auto& value = builder.Emit(IrOpcode::ReadLane, IrType::U32, {&source, &selected});
        result = &builder.Emit(IrOpcode::UMin32, IrType::U32, {result, &value});
    }
    return *result;
}
