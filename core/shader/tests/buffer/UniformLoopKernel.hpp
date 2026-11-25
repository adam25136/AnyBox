#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
struct UniformLoopProgram {
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::IrValue* counter;
    ShaderRecompiler::IrValue* sum;
    ShaderRecompiler::IrValue* broadcast;
};

inline UniformLoopProgram UniformLoopKernel(unsigned mode = 0, bool readLane = false) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& program = fixture.program;
    auto& builder = fixture.builder;
    program.SetWaveSize(64);
    auto& entry = program.EntryBlock();
    auto& header = program.CreateBlock();
    auto& body = program.CreateBlock();
    auto& exit = program.CreateBlock();
    program.BlockOrder() = {&entry, &header, &body, &exit};
    auto& metadata = program.Metadata().blockInfo;
    metadata.resize(4);
    for (unsigned block = 0; block < 4; ++block) metadata[block].id = block;
    auto& global = fixture.GlobalIndex();
    auto& local = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::LocalInvocationIndex)), &builder.Constant(0)});
    auto& group = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::WorkgroupId)), &builder.Constant(0)});
    auto& wave = builder.ShiftRightLogical(local, builder.Constant(6));
    auto* initial = mode == 1 ? &group : mode == 2 ? &builder.BitwiseAnd(global, builder.Constant(1)) : &builder.Constant(0);
    auto* step = mode == 3 ? &builder.IAdd(builder.BitwiseAnd(global, builder.Constant(1)), builder.Constant(1)) : &builder.Constant(1);
    auto* limit = &builder.IAdd(builder.IAdd(wave, mode == 1 ? group : builder.Constant(0)), builder.Constant(1));
    if (mode == 4) limit = &fixture.Memory(IrOpcode::LoadAddressU32, builder.IMul(builder.BitwiseAnd(global, builder.Constant(15)), builder.Constant(4)), 1);
    auto& payload = builder.IAdd(builder.IMul(global, builder.Constant(17)), builder.Constant(5));
    builder.Branch(header);
    metadata[0].terminator.kind = TerminatorKind::Branch;
    metadata[0].terminator.trueBlock = header.Id();
    builder.SetInsertionPoint(header);
    auto& counter = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& sum = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& condition = builder.ULessThan(counter, *limit);
    builder.BranchConditional(condition, body, exit);
    metadata[header.Id()].condition = &condition;
    metadata[header.Id()].terminator.kind = TerminatorKind::ConditionalBranch;
    metadata[header.Id()].terminator.condition = BranchCondition::ScalarInstruction;
    metadata[header.Id()].terminator.trueBlock = body.Id();
    metadata[header.Id()].terminator.falseBlock = exit.Id();
    builder.SetInsertionPoint(body);
    auto& next = builder.IAdd(counter, *step);
    auto& selected = readLane ? builder.Emit(IrOpcode::ReadLane, IrType::U32, {&global, &counter}) : builder.Constant(0);
    auto& accumulated = builder.IAdd(builder.IAdd(sum, payload), selected);
    if (mode == 5) {
        auto& stop = builder.INotEqual(builder.BitwiseAnd(global, builder.Constant(1)), builder.Constant(0));
        builder.BranchConditional(stop, exit, header);
        metadata[body.Id()].condition = &stop;
        metadata[body.Id()].terminator.kind = TerminatorKind::ConditionalBranch;
        metadata[body.Id()].terminator.condition = BranchCondition::ScalarInstruction;
        metadata[body.Id()].terminator.trueBlock = exit.Id();
        metadata[body.Id()].terminator.falseBlock = header.Id();
    } else {
        builder.Branch(header);
        metadata[body.Id()].terminator.kind = TerminatorKind::Branch;
        metadata[body.Id()].terminator.trueBlock = header.Id();
    }
    counter.AddPhiOperand(&entry, initial);
    counter.AddPhiOperand(&body, &next);
    sum.AddPhiOperand(&entry, &builder.Constant(17));
    sum.AddPhiOperand(&body, &accumulated);
    builder.SetInsertionPoint(exit);
    auto& final = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    final.AddPhiOperand(&header, &sum);
    if (mode == 5) final.AddPhiOperand(&body, &accumulated);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IMul(global, builder.Constant(8)), 2, &final);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(builder.IMul(global, builder.Constant(8)), builder.Constant(4)), 2, &counter);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    metadata[exit.Id()].terminator.kind = TerminatorKind::Return;
    ValidateProgram(program, true);
    return {std::move(program), &counter, &sum, &selected};
}
}
