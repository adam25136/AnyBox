#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline const char* UniformName(unsigned bits) { return bits == 32 ? "xbox.MakeUniform.i32" : "xbox.MakeUniform.i64"; }

inline ShaderRecompiler::IrProgram UniformKernel(unsigned bits, unsigned waveSize, unsigned firstActive, bool uniform) {
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
    if (firstActive != 0) {
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
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* resultType = LLVMBC::Type::getIntTy(context, bits);
    auto& low = uniform ? builder.Constant(0xA55AC33C) : builder.IAdd(builder.IMul(index, builder.Constant(17)), builder.Constant(5));
    auto& high = uniform ? builder.Constant(0x81FEDCBA) : builder.BitwiseXor(index, builder.Constant(0xA5F03C69));
    auto& input = bits == 32 ? low : builder.ConstructU64(low, high);
    auto* argument = context.construct<LLVMBC::Argument>(resultType, 0, false);
    fixture.scalar.Bind(*argument, input);
    auto* signature = context.construct<LLVMBC::FunctionType>(context, resultType, LLVMBC::Vector<LLVMBC::Type*>{word, resultType});
    fixture.source.Module().add_value_name(50, UniformName(bits));
    auto* function = context.construct<LLVMBC::Function>(signature, 50, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 50), argument});
    auto& result = fixture.scalar.Get(*call);
    auto write = [&](IrValue& value) {
        auto& offset = builder.ShiftLeftLogical(index, builder.Constant(bits == 32 ? 2 : 3));
        auto& lower = bits == 32 ? value : builder.CompositeExtract(value, 0);
        fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &lower);
        if (bits == 64) fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(offset, builder.Constant(4)), 2, &builder.CompositeExtract(value, 1));
    };
    write(result);
    if (merge) {
        auto jump = [&](IrBlock& block) {
            builder.Branch(*merge);
            auto& term = program.Metadata().blockInfo[block.Id()].terminator;
            term.kind = TerminatorKind::Branch;
            term.trueBlock = merge->Id();
        };
        jump(*active);
        builder.SetInsertionPoint(*inactive);
        auto& marker = bits == 32 ? builder.Constant(0xDEADBEEF) : builder.ConstantU64(0xCAFEBABEDEADBEEF);
        write(marker);
        jump(*inactive);
        builder.SetInsertionPoint(*merge);
        program.Metadata().blockInfo[merge->Id()].terminator.kind = TerminatorKind::Return;
    } else program.Metadata().blockInfo[0].terminator.kind = TerminatorKind::Return;
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(program);
}
}
