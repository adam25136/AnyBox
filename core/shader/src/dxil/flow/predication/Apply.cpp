#include "Plan.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>
#include <algorithm>
#include <stdexcept>

namespace {
using namespace ShaderRecompiler;
using namespace AnyBox::Shader::Dxil::Flow::Predication;

void Branch(IrProgram& program, IrBlock& source, IrBlock& target) {
    auto* terminator = source.Instructions().back();
    if (terminator->Opcode() != IrOpcode::Branch && terminator->Opcode() != IrOpcode::BranchConditional) throw std::logic_error("DXIL predication requires an explicit branch terminator");
    source.RemoveInstruction(terminator);
    terminator->Invalidate();
    for (auto* old : source.Successors()) std::erase(old->Predecessors(), &source);
    source.Successors().clear();
    IrBuilder builder(program);
    builder.SetInsertionPoint(source);
    builder.Branch(target);
    auto& info = Information(program, source);
    info.condition = nullptr;
    info.terminator = Terminator{};
    info.terminator.kind = TerminatorKind::Branch;
    info.terminator.trueBlock = target.Id();
}

void SelectPhis(IrProgram& program, const Plan& plan) {
    std::vector<IrValue*> phis;
    IrValue* anchor = nullptr;
    for (auto* instruction : plan.merge->Instructions()) {
        if (instruction->IsPhi()) phis.push_back(instruction);
        else if (!anchor) anchor = instruction;
    }
    for (auto* phi : phis) {
        bool positiveFirst = phi->PhiBlock(0) == (plan.positive ? plan.positive : plan.header);
        auto* positive = phi->Argument(positiveFirst ? 0 : 1);
        auto* negative = phi->Argument(positiveFirst ? 1 : 0);
        auto opcode = phi->Type() == IrType::Bool ? IrOpcode::SelectU1 : phi->Type() == IrType::F32 ? IrOpcode::SelectF32 : IrOpcode::SelectU32;
        auto& selection = program.CreateValue(opcode, phi->Type());
        selection.AddArgument(plan.condition);
        selection.AddArgument(positive);
        selection.AddArgument(negative);
        if (anchor) plan.merge->InsertInstructionBefore(anchor, &selection);
        else plan.merge->AppendInstruction(&selection);
        phi->ReplaceAllUsesWith(&selection);
        for (auto& info : program.Metadata().blockInfo) if (info.condition == phi) info.condition = &selection;
        plan.merge->RemoveInstruction(phi);
        phi->Invalidate();
    }
}
}

void AnyBox::Shader::Dxil::Flow::Predication::Apply(IrProgram& program, const Plan& plan) {
    if (plan.positive) Mask(program, *plan.positive, *plan.condition);
    if (plan.negative) {
        auto& inverse = program.CreateValue(IrOpcode::LogicalNot, IrType::Bool);
        inverse.AddArgument(plan.condition);
        plan.header->InsertInstructionBefore(plan.header->Instructions().back(), &inverse);
        Mask(program, *plan.negative, inverse);
    }
    SelectPhis(program, plan);
    Branch(program, *plan.header, plan.positive ? *plan.positive : *plan.negative);
    if (plan.positive) Branch(program, *plan.positive, plan.negative ? *plan.negative : *plan.merge);
    if (plan.negative) Branch(program, *plan.negative, *plan.merge);
}
