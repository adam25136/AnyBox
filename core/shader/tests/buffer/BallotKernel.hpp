#pragma once

#include "AtomicKernel.hpp"
#include <optional>

namespace BufferTests {
inline ShaderRecompiler::IrProgram BallotKernel(unsigned waveSize, std::uint64_t predicateMask, std::uint64_t executionMask = UINT64_MAX, bool dxil = false, unsigned countOpcode = 0, std::optional<bool> constantPredicate = {}) {
    using namespace ShaderRecompiler;
    bool counting = countOpcode != 0;
    if (counting && (!dxil || (countOpcode != 135 && countOpcode != 136))) throw std::invalid_argument("Invalid ballot count fixture opcode");
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& program = fixture.program;
    program.SetWaveSize(waveSize);
    auto& index = fixture.GlobalIndex();
    auto& local = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::LocalInvocationIndex)), &builder.Constant(0)});
    auto& lane = builder.BitwiseAnd(local, builder.Constant(63));
    auto& upper = builder.ULessThan(lane, builder.Constant(32));
    auto& shift = builder.BitwiseAnd(lane, builder.Constant(31));
    auto condition = [&](std::uint64_t mask) -> IrValue& {
        auto& selected = builder.Select(upper, builder.Constant(static_cast<std::uint32_t>(mask)), builder.Constant(static_cast<std::uint32_t>(mask >> 32)));
        return builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(selected, shift), builder.Constant(1)), builder.Constant(0));
    };
    auto& predicate = condition(predicateMask);
    auto* active = &program.EntryBlock();
    IrBlock* inactive = nullptr;
    IrBlock* merge = nullptr;
    if (executionMask != UINT64_MAX) {
        active = &program.CreateBlock();
        inactive = &program.CreateBlock();
        merge = &program.CreateBlock();
        program.BlockOrder() = {&program.EntryBlock(), active, inactive, merge};
        program.Metadata().blockInfo.resize(4);
        auto& execution = condition(executionMask);
        builder.BranchConditional(execution, *active, *inactive);
        auto& entry = program.Metadata().blockInfo[0];
        entry.condition = &execution;
        entry.terminator.kind = TerminatorKind::ConditionalBranch;
        entry.terminator.condition = BranchCondition::ScalarInstruction;
        entry.terminator.trueBlock = active->Id();
        entry.terminator.falseBlock = inactive->Id();
        for (unsigned block = 1; block < 4; ++block) program.Metadata().blockInfo[block].id = block;
    }
    builder.SetInsertionPoint(*active);
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* resultType = counting ? word : LLVMBC::StructType::get(context, LLVMBC::Vector<LLVMBC::Type*>(dxil ? 4 : 2, word), "ballot");
    auto* predicateType = dxil ? LLVMBC::Type::getInt1Ty(context) : word;
    auto* argument = context.construct<LLVMBC::Argument>(predicateType, 0, false);
    auto& nonzero = builder.Select(predicate, builder.ShiftLeftLogical(builder.Constant(1), shift), builder.Constant(0));
    fixture.scalar.Bind(*argument, dxil ? predicate : nonzero);
    fixture.source.Module().add_value_name(25, counting ? countOpcode == 136 ? "dx.op.wavePrefixOp" : "dx.op.waveAllOp" : dxil ? "dx.op.waveActiveBallot" : "xbox.Ballot64");
    auto* signature = context.construct<LLVMBC::FunctionType>(context, resultType, LLVMBC::Vector<LLVMBC::Type*>{word, predicateType});
    auto* function = context.construct<LLVMBC::Function>(signature, 25, fixture.source.Module());
    auto* input = constantPredicate ? static_cast<LLVMBC::Value*>(LLVMBC::ConstantInt::get(predicateType, *constantPredicate)) : argument;
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, counting ? countOpcode : dxil ? 116 : 25), input});
    auto write = [&](bool evaluated) {
        auto& offset = builder.ShiftLeftLogical(index, builder.Constant(counting ? 2 : dxil ? 4 : 3));
        for (unsigned component = 0; component < (counting ? 1u : dxil ? 4u : 2u); ++component) {
            auto* extraction = counting ? static_cast<LLVMBC::Value*>(call) : context.construct<LLVMBC::ExtractValueInst>(word, call, LLVMBC::Vector<unsigned>{component});
            auto& value = evaluated ? fixture.scalar.Get(*extraction) : builder.Constant(UINT32_MAX);
            fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(offset, builder.Constant(component * 4)), 2, &value);
        }
    };
    write(true);
    if (merge) {
        auto jump = [&](IrBlock& block) {
            builder.Branch(*merge);
            auto& term = program.Metadata().blockInfo[block.Id()].terminator;
            term.kind = TerminatorKind::Branch;
            term.trueBlock = merge->Id();
        };
        jump(*active);
        builder.SetInsertionPoint(*inactive);
        write(false);
        jump(*inactive);
        builder.SetInsertionPoint(*merge);
        program.Metadata().blockInfo[merge->Id()].terminator.kind = TerminatorKind::Return;
    } else program.Metadata().blockInfo[0].terminator.kind = TerminatorKind::Return;
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(program);
}
}
