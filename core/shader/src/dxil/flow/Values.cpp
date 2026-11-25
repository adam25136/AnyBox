#include "Builder.hpp"
#include <algorithm>
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::Flow::Builder;

IrValue& Builder::phi(const IrValue& variable, State& state) {
    auto type = variable.Type() == IrType::F32 ? IrType::U32 : variable.Type();
    auto& value = output.CreateValue(IrOpcode::Phi, type);
    if (state.block->Instructions().empty()) state.block->AppendInstruction(&value);
    else state.block->InsertInstructionBefore(state.block->Instructions().front(), &value);
    return value;
}

IrValue& Builder::entry(const IrValue& variable, State& state) {
    if (auto found = state.entries.find(&variable); found != state.entries.end()) return *found->second;
    if (state.block->Predecessors().empty()) return undefinedEntry(variable, state);
    auto& value = phi(variable, state);
    IrValue* result = &value;
    if (variable.Type() == IrType::F32) {
        auto& view = output.CreateValue(IrOpcode::BitCastF32U32, IrType::F32);
        view.AddArgument(&value);
        auto& instructions = state.block->Instructions();
        auto lastPhi = std::find_if(instructions.rbegin(), instructions.rend(), [](auto* instruction) { return instruction->IsPhi(); });
        auto position = lastPhi == instructions.rend() ? instructions.begin() : lastPhi.base();
        if (position == instructions.end()) state.block->AppendInstruction(&view);
        else state.block->InsertInstructionBefore(*position, &view);
        result = &view;
    }
    state.entries.emplace(&variable, result);
    IrValue* replacement = nullptr;
    bool multiple = false;
    for (auto* predecessor : state.block->Predecessors()) {
        auto& incoming = exit(variable, states.at(ids.at(predecessor)));
        IrValue* bits = &incoming;
        if (variable.Type() == IrType::F32) {
            IrBuilder builder(output);
            builder.SetInsertionPoint(*predecessor);
            bits = &builder.BitCastU32(incoming);
        }
        value.AddPhiOperand(predecessor, bits);
        auto* resolved = bits->Resolve();
        if (resolved == &value) continue;
        if (!replacement) replacement = resolved;
        else if (replacement != resolved) multiple = true;
    }
    if (replacement && !multiple) value.ReplaceUsesWith(replacement, true);
    return *result->Resolve();
}

IrValue& Builder::exit(const IrValue& variable, State& state) {
    if (auto found = state.definitions.find(&variable); found != state.definitions.end()) return *found->second;
    return entry(variable, state);
}

void Builder::connectInstructions() {
    for (auto& state : states) {
        for (auto [original, copy] : state.instructions) {
            for (auto* argument : original->Arguments()) copy->AddArgument(&operand(*argument, state));
        }
        if (!state.origin) continue;
        for (const auto& assignment : state.origin->assignments) {
            auto* value = assignment.source->Resolve();
            auto& incoming = value->HasImmediate() ? constant(*value) : entry(*value, state);
            state.definitions.at(assignment.target)->ReplaceUsesWith(&incoming, true);
        }
    }
}

void Builder::connectConditions() {
    for (auto& info : output.Metadata().blockInfo) {
        auto& state = states.at(info.id);
        IrBuilder builder(output);
        builder.SetInsertionPoint(*state.block);
        const auto& term = info.terminator;
        if (term.gotoValue >= 0) builder.SetGotoVariable(term.gotoVariable, builder.ConstantBool(term.gotoValue != 0));
        if (term.kind != TerminatorKind::ConditionalBranch) continue;
        if (term.condition == BranchCondition::GotoVariable) info.condition = &builder.GetGotoVariable(term.gotoVariable);
        else if (term.condition == BranchCondition::ScalarInstruction && state.origin && state.origin->condition) info.condition = &operand(*state.origin->condition, state);
        else throw std::runtime_error("DXIL structured branch condition is not implemented");
        (void)builder.Emit(IrOpcode::Reference, IrType::Void, {info.condition});
    }
}
