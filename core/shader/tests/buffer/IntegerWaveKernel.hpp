#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeIntegerWaveKernel(unsigned waveSize, bool prefix, std::uint32_t mask, unsigned signedness = 1) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& program = fixture.program;
    program.SetWaveSize(waveSize);
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& offset = builder.ShiftLeftLogical(builder.BitwiseAnd(lane, builder.Constant(15)), builder.Constant(2));
    auto& input = fixture.Memory(IrOpcode::LoadAddressU32, offset, 1);
    auto& shift = builder.BitwiseAnd(lane, builder.Constant(31));
    auto& active = builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(builder.Constant(mask), shift), builder.Constant(1)), builder.Constant(0));
    if (waveSize == 32) {
        (void)builder.Emit(IrOpcode::Reference, IrType::Void, {&active});
        auto& entry = program.EntryBlock();
        auto& body = program.CreateBlock();
        auto& exit = program.CreateBlock();
        program.BlockOrder() = {&entry, &body, &exit};
        program.Metadata().blockInfo.resize(3);
        for (unsigned id = 0; id < 3; ++id) program.Metadata().blockInfo[id].id = id;
        entry.AddBranch(&body);
        entry.AddBranch(&exit);
        body.AddBranch(&exit);
        auto& info = program.Metadata().blockInfo[0];
        info.condition = &active;
        info.terminator.kind = TerminatorKind::ConditionalBranch;
        info.terminator.condition = BranchCondition::ScalarInstruction;
        info.terminator.trueBlock = 1;
        info.terminator.falseBlock = info.terminator.mergeBlock = 2;
        auto& branch = program.Metadata().blockInfo[1].terminator;
        branch.kind = TerminatorKind::Branch;
        branch.trueBlock = 2;
        builder.SetInsertionPoint(body);
    }
    auto& context = fixture.source.Context();
    auto& symbols = fixture.source.Module();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* byte = LLVMBC::Type::getInt8Ty(context);
    auto* argument = context.construct<LLVMBC::Argument>(word, 0, false);
    fixture.scalar.Bind(*argument, input);
    symbols.add_value_name(119, prefix ? "dx.op.wavePrefixOp.i32" : "dx.op.waveActiveOp.i32");
    auto* signature = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{word, word, byte, byte});
    auto* function = context.construct<LLVMBC::Function>(signature, 119, symbols);
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, prefix ? 121 : 119), argument, LLVMBC::ConstantInt::get(byte, 0), LLVMBC::ConstantInt::get(byte, signedness)});
    auto& result = fixture.scalar.Get(*call);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &result);
    if (waveSize == 64) (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    ValidateProgram(program, true);
    return std::move(program);
}
}
