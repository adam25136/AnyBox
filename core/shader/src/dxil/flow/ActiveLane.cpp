#include "ActiveLane.hpp"
#include <IntermediateRepresentation/IrProgram.hpp>

namespace {
using namespace ShaderRecompiler;

bool Constant(const IrValue& value, std::uint32_t expected) {
    auto& resolved = *value.Resolve();
    return resolved.Type() == IrType::U32 && resolved.HasImmediate() && resolved.ImmediateU32() == expected;
}

const IrValue* Node(const IrValue& value, IrOpcode opcode, unsigned arguments, const IrBlock* block) {
    auto* resolved = value.Resolve();
    return resolved->Opcode() == opcode && resolved->ArgumentCount() == arguments && resolved->Parent() == block ? resolved : nullptr;
}
}

bool AnyBox::Shader::Dxil::Flow::ReadsActiveLane(const IrValue& read, unsigned waveSize) {
    if (waveSize != 32 || read.Opcode() != IrOpcode::ReadLane || read.ArgumentCount() != 2 || !read.Parent()) return false;
    auto* block = read.Parent();
    auto* select = Node(*read.Argument(1), IrOpcode::SelectU32, 3, block);
    if (!select || !select->Argument(1)->HasImmediate() || select->Argument(1)->Type() != IrType::U32) return false;
    auto lane = select->Argument(1)->ImmediateU32();
    if (lane >= 32) return false;
    auto* present = Node(*select->Argument(0), IrOpcode::INotEqual32, 2, block);
    if (!present || !Constant(*present->Argument(1), 0)) return false;
    auto* bit = Node(*present->Argument(0), IrOpcode::BitwiseAnd32, 2, block);
    if (!bit || !Constant(*bit->Argument(1), std::uint32_t{1} << lane)) return false;
    auto* first = Node(*select->Argument(2), IrOpcode::FindILsb32, 1, block);
    if (!first || first->Argument(0)->Resolve() != bit->Argument(0)->Resolve()) return false;
    auto* low = Node(*bit->Argument(0), IrOpcode::CompositeExtractU32x4, 2, block);
    if (!low || !Constant(*low->Argument(1), 0)) return false;
    auto* ballot = Node(*low->Argument(0), IrOpcode::Ballot, 1, block);
    if (!ballot) return false;
    auto* predicate = ballot->Argument(0)->Resolve();
    if (!predicate->HasImmediate() || predicate->Type() != IrType::Bool || !predicate->ImmediateBool()) return false;

    bool seen = false;
    for (auto* instruction : block->Instructions()) {
        if (instruction == ballot) seen = true;
        if (instruction == &read) return seen;
    }
    return false;
}
