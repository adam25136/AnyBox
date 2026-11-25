#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram WaveControlKernel(unsigned mode) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& program = fixture.program;
    auto& builder = fixture.builder;
    program.SetWaveSize(64);
    auto& entry = program.EntryBlock();
    auto& body = program.CreateBlock();
    auto& alternate = program.CreateBlock();
    auto& merge = program.CreateBlock();
    program.BlockOrder() = {&entry, &body, &alternate, &merge};
    auto& metadata = program.Metadata().blockInfo;
    metadata.resize(4);
    for (unsigned block = 0; block < 4; ++block) metadata[block].id = block;
    auto& index = fixture.GlobalIndex();
    IrValue* source = &index;
    if (mode == 2) source = &builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::WorkgroupId)), &builder.Constant(0)});
    if (mode == 3) source = &builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&index, &builder.ConstantBool(true)});
    if (mode == 5) source = &fixture.Memory(IrOpcode::LoadAddressU32, builder.Constant(0), 1);
    auto& condition = mode < 2 ? builder.ConstantBool(mode != 0) : builder.INotEqual(builder.BitwiseAnd(*source, builder.Constant(mode == 2 ? 1 : mode == 3 ? 64 : 32)), builder.Constant(0));
    builder.BranchConditional(condition, body, alternate);
    metadata[0].condition = &condition;
    metadata[0].terminator.kind = TerminatorKind::ConditionalBranch;
    metadata[0].terminator.condition = BranchCondition::ScalarInstruction;
    metadata[0].terminator.trueBlock = body.Id();
    metadata[0].terminator.falseBlock = alternate.Id();
    auto finish = [&](IrBlock& block, IrValue& value) {
        fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &value);
        builder.Branch(merge);
        auto& term = metadata[block.Id()].terminator;
        term.kind = TerminatorKind::Branch;
        term.trueBlock = merge.Id();
    };
    builder.SetInsertionPoint(body);
    auto& first = builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&index, &builder.ConstantBool(true)});
    finish(body, builder.IAdd(index, first));
    builder.SetInsertionPoint(alternate);
    finish(alternate, builder.BitwiseXor(index, builder.Constant(0xA55A8420)));
    builder.SetInsertionPoint(merge);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    metadata[merge.Id()].terminator.kind = TerminatorKind::Return;
    ValidateProgram(program, true);
    return std::move(program);
}
}
