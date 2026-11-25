#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeSpannedControlKernel(bool prefix) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& program = fixture.program;
    auto& builder = fixture.builder;
    program.SetWaveSize(64);
    auto& index = fixture.GlobalIndex();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* byte = LLVMBC::Type::getInt8Ty(context);
    auto* value = context.construct<LLVMBC::Argument>(word, 0, false);
    fixture.scalar.Bind(*value, builder.IAdd(index, builder.Constant(1)));
    fixture.source.Module().add_value_name(119, prefix ? "dx.op.wavePrefixOp.i32" : "dx.op.waveActiveOp.i32");
    auto* type = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{word, word, byte, byte});
    auto* function = context.construct<LLVMBC::Function>(type, 119, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, prefix ? 121 : 119), value, LLVMBC::ConstantInt::get(byte, 0), LLVMBC::ConstantInt::get(byte, 1)});
    auto& reduced = fixture.scalar.Get(*call);
    auto& local = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::LocalInvocationIndex)), &builder.Constant(0)});
    auto& upper = builder.Emit(IrOpcode::UGreaterThanEqual32, IrType::Bool, {&local, &builder.Constant(32)});
    auto& limit = builder.IAdd(builder.BitwiseAnd(local, builder.Constant(3)), builder.Select(upper, builder.Constant(4), builder.Constant(1)));
    auto& entry = program.EntryBlock();
    auto& header = program.CreateBlock();
    auto& body = program.CreateBlock();
    auto& exit = program.CreateBlock();
    program.BlockOrder() = {&entry, &header, &body, &exit};
    auto& metadata = program.Metadata().blockInfo;
    metadata.resize(4);
    for (unsigned block = 0; block < 4; ++block) metadata[block].id = block;
    builder.Branch(header);
    metadata[0].terminator.kind = TerminatorKind::Branch;
    metadata[0].terminator.trueBlock = header.Id();
    builder.SetInsertionPoint(header);
    auto& counter = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& sum = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& condition = builder.ULessThan(counter, limit);
    builder.BranchConditional(condition, body, exit);
    metadata[1].condition = &condition;
    metadata[1].terminator.kind = TerminatorKind::ConditionalBranch;
    metadata[1].terminator.condition = BranchCondition::ScalarInstruction;
    metadata[1].terminator.trueBlock = body.Id();
    metadata[1].terminator.falseBlock = exit.Id();
    builder.SetInsertionPoint(body);
    auto& next = builder.IAdd(counter, builder.Constant(1));
    auto& updated = builder.IAdd(sum, builder.IAdd(index, counter));
    builder.Branch(header);
    metadata[2].terminator.kind = TerminatorKind::Branch;
    metadata[2].terminator.trueBlock = header.Id();
    counter.AddPhiOperand(&entry, &builder.Constant(0));
    counter.AddPhiOperand(&body, &next);
    sum.AddPhiOperand(&entry, &reduced);
    sum.AddPhiOperand(&body, &updated);
    builder.SetInsertionPoint(exit);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &sum);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    metadata[3].terminator.kind = TerminatorKind::Return;
    ValidateProgram(program, true);
    return std::move(program);
}
}
