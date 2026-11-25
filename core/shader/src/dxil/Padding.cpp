#include "Padding.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>
#include <Optimization/ShaderStageInputInfo.hpp>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <string>

namespace {
using namespace ShaderRecompiler;

void Mask(IrProgram& program, IrBlock& block, IrValue& instruction, std::size_t index, IrValue& active) {
    auto* predicate = instruction.Argument(index)->Resolve();
    if (predicate->Type() != IrType::Bool) throw std::runtime_error("DXIL padded memory operation has no boolean activity operand");
    if (predicate->HasImmediate()) {
        if (predicate->ImmediateBool()) instruction.ReplaceArgument(index, &active);
        return;
    }
    auto& combined = program.CreateValue(IrOpcode::LogicalAnd, IrType::Bool);
    combined.AddArgument(predicate);
    combined.AddArgument(&active);
    block.InsertInstructionBefore(&instruction, &combined);
    instruction.ReplaceArgument(index, &combined);
}
}

void AnyBox::Shader::Dxil::MaskComputePadding(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input) {
    if (input.waveSize != 64 || input.hostSubgroupSize != 32) return;
    std::uint64_t threads = 1;
    for (auto size : input.threadsNum) {
        if (size == 0 || size > 1024) throw std::invalid_argument("DXIL padded workgroup dimension is invalid");
        threads *= size;
    }
    if (threads > 1024) throw std::invalid_argument("DXIL padded workgroup has too many invocations");
    if (threads % 64 == 0) return;
    auto& entry = program.EntryBlock();
    IrBuilder constants(program);
    auto& index = program.CreateValue(IrOpcode::GetBuiltin, IrType::U32);
    index.AddArgument(&constants.Constant(static_cast<unsigned>(StageInputKind::LocalInvocationIndex)));
    index.AddArgument(&constants.Constant(0));
    auto& active = program.CreateValue(IrOpcode::ULessThan32, IrType::Bool);
    active.AddArgument(&index);
    active.AddArgument(&constants.Constant(static_cast<std::uint32_t>(threads)));
    auto first = std::find_if(entry.Instructions().begin(), entry.Instructions().end(), [](auto* value) { return !value->IsPhi(); });
    if (first == entry.Instructions().end()) {
        entry.AppendInstruction(&index);
        entry.AppendInstruction(&active);
    } else {
        auto* position = *first;
        entry.InsertInstructionBefore(position, &index);
        entry.InsertInstructionBefore(position, &active);
    }
    for (auto* block : program.BlockOrder()) {
        std::vector<IrValue*> original(block->Instructions().begin(), block->Instructions().end());
        for (auto* instruction : original) {
            auto opcode = instruction->Opcode();
            if (AddressOpcodeInfoOf(opcode).access != AddressAccess::None || SharedAccessOf(opcode) != SharedAccess::None) {
                if (instruction->ArgumentCount() == 0) throw std::runtime_error("DXIL padded memory operation has no activity operand");
                Mask(program, *block, *instruction, instruction->ArgumentCount() - 1, active);
            } else if (opcode == IrOpcode::Ballot || opcode == IrOpcode::DescriptorFault) {
                Mask(program, *block, *instruction, 0, active);
            } else if (opcode == IrOpcode::ReadFirstLane) {
                Mask(program, *block, *instruction, 1, active);
            } else if (opcode == IrOpcode::SwizzleU32) {
                Mask(program, *block, *instruction, 2, active);
            } else if (ImageOpcodeInfoOf(opcode).access != ImageAccess::None) {
                throw std::runtime_error("DXIL padded image operations are not implemented");
            } else if (IrOpcodeHasSideEffects(opcode) && opcode != IrOpcode::Reference && opcode != IrOpcode::Branch && opcode != IrOpcode::BranchConditional && opcode != IrOpcode::Return && opcode != IrOpcode::Barrier && opcode != IrOpcode::SetGotoVariable) {
                throw std::runtime_error("DXIL padded compute side effect is not implemented: " + std::string(IrOpcodeName(opcode)));
            }
        }
    }
}
