#include "Builder.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::Flow::Builder;

IrValue& Builder::undefinedEntry(const IrValue& variable, State& state) {
    if (state.block != &output.EntryBlock() || !variable.Parent() || variable.Parent() == &source.EntryBlock()) throw std::runtime_error("DXIL structured SSA reads a value before its definition");
    IrOpcode opcode;
    switch (variable.Type()) {
        case IrType::Bool: opcode = IrOpcode::UndefU1; break;
        case IrType::U8: opcode = IrOpcode::UndefU8; break;
        case IrType::U16: opcode = IrOpcode::UndefU16; break;
        case IrType::F16: opcode = IrOpcode::UndefU16; break;
        case IrType::U32:
        case IrType::F32: opcode = IrOpcode::UndefU32; break;
        case IrType::U64: opcode = IrOpcode::UndefU64; break;
        default: throw std::runtime_error("DXIL structured SSA undefined entry type is not implemented");
    }
    auto type = variable.Type() == IrType::F32 ? IrType::U32 : variable.Type() == IrType::F16 ? IrType::U16 : variable.Type();
    auto& value = output.CreateValue(opcode, type);
    state.block->AppendInstruction(&value);
    auto* result = &value;
    if (variable.Type() == IrType::F32 || variable.Type() == IrType::F16) {
        auto conversion = variable.Type() == IrType::F32 ? IrOpcode::BitCastF32U32 : IrOpcode::BitCastF16U16;
        auto& view = output.CreateValue(conversion, variable.Type());
        view.AddArgument(&value);
        state.block->AppendInstruction(&view);
        result = &view;
    }
    state.entries.emplace(&variable, result);
    return *result;
}
