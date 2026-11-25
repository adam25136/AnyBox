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

bool WordOf(const IrValue& value, const IrValue& ballot, unsigned component, const IrBlock* block) {
    auto* word = Node(value, IrOpcode::CompositeExtractU32x4, 2, block);
    return word && word->Argument(0)->Resolve() == &ballot && Constant(*word->Argument(1), component);
}

bool First64(const IrValue& value, const IrValue& ballot, const IrBlock* block) {
    auto* selection = Node(value, IrOpcode::SelectU32, 3, block);
    if (!selection) return false;
    auto* emptyLow = Node(*selection->Argument(0), IrOpcode::IEqual32, 2, block);
    if (!emptyLow || !Constant(*emptyLow->Argument(1), 0) || !WordOf(*emptyLow->Argument(0), ballot, 0, block)) return false;
    auto* upper = Node(*selection->Argument(1), IrOpcode::IAdd32, 2, block);
    auto* lowFirst = Node(*selection->Argument(2), IrOpcode::FindILsb32, 1, block);
    if (!upper || !lowFirst || !Constant(*upper->Argument(1), 32)) return false;
    auto* highFirst = Node(*upper->Argument(0), IrOpcode::FindILsb32, 1, block);
    return highFirst && WordOf(*highFirst->Argument(0), ballot, 1, block) && WordOf(*lowFirst->Argument(0), ballot, 0, block);
}
}

bool AnyBox::Shader::Dxil::Flow::ReadsActiveLane(const IrValue& read, unsigned waveSize) {
    if ((waveSize != 32 && waveSize != 64) || read.Opcode() != IrOpcode::ReadLane || read.ArgumentCount() != 2 || !read.Parent()) return false;
    auto* block = read.Parent();
    auto* select = Node(*read.Argument(1), IrOpcode::SelectU32, 3, block);
    if (!select || !select->Argument(1)->HasImmediate() || select->Argument(1)->Type() != IrType::U32) return false;
    auto lane = select->Argument(1)->ImmediateU32();
    if (lane >= waveSize) return false;
    auto* present = Node(*select->Argument(0), IrOpcode::INotEqual32, 2, block);
    if (!present || !Constant(*present->Argument(1), 0)) return false;
    auto* bit = Node(*present->Argument(0), IrOpcode::BitwiseAnd32, 2, block);
    if (!bit || !Constant(*bit->Argument(1), std::uint32_t{1} << (lane & 31))) return false;
    auto* low = Node(*bit->Argument(0), IrOpcode::CompositeExtractU32x4, 2, block);
    if (!low || !Constant(*low->Argument(1), lane / 32)) return false;
    auto* ballot = Node(*low->Argument(0), IrOpcode::Ballot, 1, block);
    if (!ballot) return false;
    auto* predicate = ballot->Argument(0)->Resolve();
    if (!predicate->HasImmediate() || predicate->Type() != IrType::Bool || !predicate->ImmediateBool()) return false;
    if (waveSize == 64) {
        if (!First64(*select->Argument(2), *ballot, block)) return false;
    } else {
        auto* first = Node(*select->Argument(2), IrOpcode::FindILsb32, 1, block);
        if (!first || first->Argument(0)->Resolve() != bit->Argument(0)->Resolve()) return false;
    }

    bool seen = false;
    for (auto* instruction : block->Instructions()) {
        if (instruction == ballot) seen = true;
        if (instruction == &read) return seen;
    }
    return false;
}
