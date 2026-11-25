#pragma once

#include "AtomicKernel.hpp"
#include <optional>

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeIntegerWaveKernel(unsigned waveSize, bool prefix, std::uint32_t mask, unsigned signedness = 1, bool dxilFlow = false, unsigned bits = 32, std::optional<std::uint64_t> constantValue = {}) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& program = fixture.program;
    program.SetWaveSize(waveSize);
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& offset = builder.ShiftLeftLogical(builder.BitwiseAnd(lane, builder.Constant(bits == 64 ? 7 : 15)), builder.Constant(bits == 64 ? 3 : 2));
    auto& low = constantValue ? builder.Constant(static_cast<std::uint32_t>(*constantValue)) : fixture.Memory(IrOpcode::LoadAddressU32, offset, 1);
    auto& high = constantValue ? builder.Constant(static_cast<std::uint32_t>(*constantValue >> 32)) : bits == 64 ? fixture.Memory(IrOpcode::LoadAddressU32, builder.IAdd(offset, builder.Constant(4)), 1) : builder.Constant(0);
    auto& input = bits == 64 ? builder.ConstructU64(low, high) : low;
    auto& shift = builder.BitwiseAnd(lane, builder.Constant(31));
    auto& active = builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(builder.Constant(mask), shift), builder.Constant(1)), builder.Constant(0));
    if (waveSize == 32) {
        if (!dxilFlow) (void)builder.Emit(IrOpcode::Reference, IrType::Void, {&active});
        auto& entry = program.EntryBlock();
        auto& body = program.CreateBlock();
        auto& exit = program.CreateBlock();
        program.BlockOrder() = {&entry, &body, &exit};
        program.Metadata().blockInfo.resize(3);
        for (unsigned id = 0; id < 3; ++id) program.Metadata().blockInfo[id].id = id;
        if (dxilFlow) builder.BranchConditional(active, body, exit);
        else {
            entry.AddBranch(&body);
            entry.AddBranch(&exit);
            body.AddBranch(&exit);
        }
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
    auto* resultType = LLVMBC::Type::getIntTy(context, bits);
    auto* argument = context.construct<LLVMBC::Argument>(resultType, 0, false);
    fixture.scalar.Bind(*argument, input);
    symbols.add_value_name(119, bits == 64 ? prefix ? "dx.op.wavePrefixOp.i64" : "dx.op.waveActiveOp.i64" : prefix ? "dx.op.wavePrefixOp.i32" : "dx.op.waveActiveOp.i32");
    auto* signature = context.construct<LLVMBC::FunctionType>(context, resultType, LLVMBC::Vector<LLVMBC::Type*>{word, resultType, byte, byte});
    auto* function = context.construct<LLVMBC::Function>(signature, 119, symbols);
    auto* payload = constantValue ? static_cast<LLVMBC::Value*>(LLVMBC::ConstantInt::get(resultType, *constantValue)) : argument;
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, prefix ? 121 : 119), payload, LLVMBC::ConstantInt::get(byte, 0), LLVMBC::ConstantInt::get(byte, signedness)});
    auto& result = fixture.scalar.Get(*call);
    auto& destination = builder.ShiftLeftLogical(index, builder.Constant(bits == 64 ? 3 : 2));
    auto& resultLow = bits == 64 ? builder.CompositeExtract(result, 0) : result;
    fixture.Memory(IrOpcode::StoreAddressU32, destination, 2, &resultLow);
    if (bits == 64) fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(destination, builder.Constant(4)), 2, &builder.CompositeExtract(result, 1));
    if (dxilFlow && waveSize == 32) {
        builder.Branch(*program.BlockOrder().back());
        builder.SetInsertionPoint(*program.BlockOrder().back());
        program.Metadata().blockInfo.back().terminator.kind = TerminatorKind::Return;
        (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    }
    if (waveSize == 64) (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    ValidateProgram(program, true);
    return std::move(program);
}
}
