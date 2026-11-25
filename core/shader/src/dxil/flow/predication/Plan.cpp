#include "Plan.hpp"
#include "../../../backend/WaveControl.hpp"
#include <Optimization/ShaderStageInputInfo.hpp>
#include <algorithm>
#include <stdexcept>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader::Dxil::Flow::Predication;

BlockInfo& AnyBox::Shader::Dxil::Flow::Predication::Information(IrProgram& program, const IrBlock& block) {
    auto& metadata = program.Metadata().blockInfo;
    auto found = std::ranges::find(metadata, block.Id(), &BlockInfo::id);
    if (found == metadata.end()) throw std::logic_error("DXIL predication block metadata is missing");
    return *found;
}

std::optional<Plan> AnyBox::Shader::Dxil::Flow::Predication::Find(IrProgram& program, IrBlock& header) {
    const auto& info = Information(program, header);
    const auto& term = info.terminator;
    if (term.kind != TerminatorKind::ConditionalBranch || term.condition != BranchCondition::ScalarInstruction || !info.condition || header.Successors().size() != 2) return {};
    if (info.condition->Type() != IrType::Bool || header.Instructions().empty() || header.Instructions().back()->Opcode() != IrOpcode::BranchConditional) return {};
    auto target = [&](std::uint32_t id) {
        for (auto* block : header.Successors()) if (block->Id() == id) return block;
        throw std::logic_error("DXIL predication branch target is missing");
    };
    auto* positive = target(term.trueBlock);
    auto* negative = target(term.falseBlock);
    if (positive == negative || positive == &header || negative == &header) return {};
    auto exit = [&](IrBlock* block) -> IrBlock* {
        if (block->Predecessors().size() != 1 || block->Predecessors().front() != &header || block->Successors().size() != 1 || Information(program, *block).terminator.kind != TerminatorKind::Branch) return nullptr;
        if (block->Instructions().empty() || block->Instructions().back()->Opcode() != IrOpcode::Branch) return nullptr;
        return block->Successors().front();
    };
    auto* positiveExit = exit(positive);
    auto* negativeExit = exit(negative);
    IrBlock* merge = nullptr;
    if (positiveExit == negative) { merge = negative; negative = nullptr; }
    else if (negativeExit == positive) { merge = positive; positive = nullptr; }
    else if (positiveExit && positiveExit == negativeExit) merge = positiveExit;
    if (!merge || merge == &header || merge->Predecessors().size() != 2) return {};
    if ((positive && !CanMask(*positive)) || (negative && !CanMask(*negative))) return {};
    auto* positivePredecessor = positive ? positive : &header;
    auto* negativePredecessor = negative ? negative : &header;
    for (auto* instruction : merge->Instructions()) {
        if (!instruction->IsPhi()) continue;
        if (instruction->Type() != IrType::Bool && instruction->Type() != IrType::U32 && instruction->Type() != IrType::F32) return {};
        if (instruction->PhiBlockCount() != 2 || instruction->ArgumentCount() != 2) return {};
        bool matched = (instruction->PhiBlock(0) == positivePredecessor && instruction->PhiBlock(1) == negativePredecessor) || (instruction->PhiBlock(1) == positivePredecessor && instruction->PhiBlock(0) == negativePredecessor);
        if (!matched) return {};
    }
    return Plan{&header, positive, negative, merge, info.condition};
}

std::size_t AnyBox::Shader::Dxil::Flow::Predication::Run(IrProgram& program, const ShaderComputeInputInfo& input) {
    if (input.waveSize != 64 || input.hostSubgroupSize != 32) return 0;
    ValidateProgram(program, true);
    std::size_t changed = 0;
    for (auto* block : program.BlockOrder()) {
        auto plan = Find(program, *block);
        if (!plan || AnyBox::Shader::IsWaveUniform(*plan->condition, input)) continue;
        Apply(program, *plan);
        ++changed;
    }
    ValidateProgram(program, true);
    return changed;
}
