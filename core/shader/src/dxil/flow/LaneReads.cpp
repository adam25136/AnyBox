#include "LaneReads.hpp"
#include "../../backend/WaveControl.hpp"
#include <IntermediateRepresentation/IrProgram.hpp>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>

namespace {
using namespace ShaderRecompiler;
using BlockMap = std::unordered_map<std::uint32_t, const IrBlock*>;

bool Mark(const BlockMap& blocks, std::uint32_t start, std::uint32_t stop, std::unordered_set<const IrBlock*>& marked) {
    std::vector<std::uint32_t> pending{start};
    std::unordered_set<std::uint32_t> visited;
    bool returns = false;
    while (!pending.empty()) {
        auto id = pending.back();
        pending.pop_back();
        if (id == stop || !visited.insert(id).second) continue;
        auto* block = blocks.at(id);
        marked.insert(block);
        if (block->Successors().empty()) returns = true;
        for (auto* successor : block->Successors()) pending.push_back(successor->Id());
    }
    return returns;
}
}

void AnyBox::Shader::Dxil::Flow::ValidateLaneReads(const IrProgram& program, const ShaderComputeInputInfo& input) {
    std::vector<const IrValue*> reads;
    BlockMap blocks;
    for (auto* block : program.BlockOrder()) {
        blocks.emplace(block->Id(), block);
        for (auto* instruction : block->Instructions()) if (instruction->Opcode() == IrOpcode::ReadLane) reads.push_back(instruction);
    }
    if (reads.empty()) return;
    std::unordered_set<const IrBlock*> divergent;
    for (const auto& info : program.Metadata().blockInfo) {
        const auto& term = info.terminator;
        if (term.kind != TerminatorKind::ConditionalBranch) continue;
        if (term.condition == BranchCondition::ScalarInstruction && info.condition && IsWaveUniform(*info.condition, input)) continue;
        bool returns = Mark(blocks, term.trueBlock, term.mergeBlock, divergent);
        returns |= Mark(blocks, term.falseBlock, term.mergeBlock, divergent);
        if (returns && term.mergeBlock != InvalidControlFlowId) (void)Mark(blocks, term.mergeBlock, InvalidControlFlowId, divergent);
    }
    for (auto* read : reads) {
        if (read->ArgumentCount() != 2 || !IsWaveUniform(*read->Argument(1), input)) throw std::runtime_error("Xbox read lane requires a wave-uniform selector");
        if (divergent.contains(read->Parent())) throw std::runtime_error("Xbox read lane from divergent control flow is not implemented");
    }
}
