#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram SwizzleKernel(unsigned waveSize, std::span<const std::uint32_t> patterns, unsigned firstActive = 0) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& program = fixture.program;
    program.SetWaveSize(waveSize);
    auto& index = fixture.GlobalIndex();
    auto* active = &program.EntryBlock();
    IrBlock* inactive = nullptr;
    IrBlock* merge = nullptr;
    if (firstActive) {
        active = &program.CreateBlock();
        inactive = &program.CreateBlock();
        merge = &program.CreateBlock();
        program.BlockOrder() = {&program.EntryBlock(), active, inactive, merge};
        program.Metadata().blockInfo.resize(4);
        auto& lane = builder.BitwiseAnd(index, builder.Constant(waveSize - 1));
        auto& condition = builder.LogicalNot(builder.ULessThan(lane, builder.Constant(firstActive)));
        builder.BranchConditional(condition, *active, *inactive);
        auto& entry = program.Metadata().blockInfo[0];
        entry.condition = &condition;
        entry.terminator.kind = TerminatorKind::ConditionalBranch;
        entry.terminator.condition = BranchCondition::ScalarInstruction;
        entry.terminator.trueBlock = active->Id();
        entry.terminator.falseBlock = inactive->Id();
        for (unsigned block = 1; block < 4; ++block) program.Metadata().blockInfo[block].id = block;
    }
    builder.SetInsertionPoint(*active);
    auto& payload = builder.IAdd(builder.IMul(index, builder.Constant(17)), builder.Constant(5));
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* argument = context.construct<LLVMBC::Argument>(word, 0, false);
    fixture.scalar.Bind(*argument, payload);
    fixture.source.Module().add_value_name(53, "xbox.LaneSwizzle.i32");
    auto* signature = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{word, word, word});
    auto* function = context.construct<LLVMBC::Function>(signature, 53, fixture.source.Module());
    auto write = [&](unsigned item, IrValue& result) {
        auto& offset = builder.IMul(builder.IAdd(builder.IMul(index, builder.Constant(static_cast<std::uint32_t>(patterns.size()))), builder.Constant(item)), builder.Constant(4));
        fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &result);
    };
    for (unsigned item = 0; item < patterns.size(); ++item) {
        auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 53), argument, LLVMBC::ConstantInt::get(word, patterns[item])});
        write(item, fixture.scalar.Get(*call));
    }
    if (merge) {
        auto jump = [&](IrBlock& block) {
            builder.Branch(*merge);
            auto& term = program.Metadata().blockInfo[block.Id()].terminator;
            term.kind = TerminatorKind::Branch;
            term.trueBlock = merge->Id();
        };
        jump(*active);
        builder.SetInsertionPoint(*inactive);
        for (unsigned item = 0; item < patterns.size(); ++item) write(item, builder.Constant(UINT32_MAX));
        jump(*inactive);
        builder.SetInsertionPoint(*merge);
        program.Metadata().blockInfo[merge->Id()].terminator.kind = TerminatorKind::Return;
    } else program.Metadata().blockInfo[0].terminator.kind = TerminatorKind::Return;
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(program);
}
}
