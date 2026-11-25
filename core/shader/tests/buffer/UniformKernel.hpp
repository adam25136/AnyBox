#pragma once

#include "AtomicKernel.hpp"
#include <optional>

namespace BufferTests {
enum class LaneSelectorMode { Constant, Own, Xor, Reverse };
struct IndexedLaneSelector { LaneSelectorMode mode; unsigned lane = 0; };
inline const char* UniformName(unsigned bits) { return bits == 32 ? "xbox.MakeUniform.i32" : "xbox.MakeUniform.i64"; }
inline const char* FirstLaneName(unsigned bits, bool floating) { return floating ? "dx.op.waveReadLaneFirst.f32" : bits == 1 ? "dx.op.waveReadLaneFirst.i1" : bits == 64 ? "dx.op.waveReadLaneFirst.i64" : "dx.op.waveReadLaneFirst.i32"; }
inline const char* IndexedLaneName(unsigned bits, bool floating) { return floating ? "dx.op.waveReadLaneAt.f32" : bits == 1 ? "dx.op.waveReadLaneAt.i1" : bits == 64 ? "dx.op.waveReadLaneAt.i64" : "dx.op.waveReadLaneAt.i32"; }

inline ShaderRecompiler::IrProgram UniformKernel(unsigned bits, unsigned waveSize, unsigned firstActive, bool uniform, bool dxil = false, bool floating = false, std::optional<IndexedLaneSelector> indexed = {}) {
    using namespace ShaderRecompiler;
    if ((!dxil && (bits != 32 && bits != 64)) || (floating && (!dxil || bits != 32))) throw std::invalid_argument("Invalid first lane fixture overload");
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
    auto* resultType = floating ? LLVMBC::Type::getFloatTy(context) : LLVMBC::Type::getIntTy(context, bits);
    auto& low = uniform ? builder.Constant(0xA55AC33C) : builder.IAdd(builder.IMul(index, builder.Constant(17)), builder.Constant(5));
    auto& high = uniform ? builder.Constant(0x81FEDCBA) : builder.BitwiseXor(index, builder.Constant(0xA5F03C69));
    auto& input = bits == 1 ? (uniform ? builder.ConstantBool(true) : builder.INotEqual(builder.BitwiseAnd(index, builder.Constant(65)), builder.Constant(0))) : bits == 64 ? builder.ConstructU64(low, high) : floating ? builder.BitCastF32(low) : low;
    auto* argument = context.construct<LLVMBC::Argument>(resultType, 0, false);
    fixture.scalar.Bind(*argument, input);
    LLVMBC::Vector<LLVMBC::Type*> parameters{word, resultType};
    LLVMBC::Vector<LLVMBC::Value*> arguments{LLVMBC::ConstantInt::get(word, indexed ? 117 : dxil ? 118 : 50), argument};
    if (indexed) {
        parameters.push_back(word);
        if (indexed->mode == LaneSelectorMode::Constant) arguments.push_back(LLVMBC::ConstantInt::get(word, indexed->lane));
        else {
            auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
            auto& selector = indexed->mode == LaneSelectorMode::Own ? lane : indexed->mode == LaneSelectorMode::Xor ? builder.BitwiseXor(lane, builder.Constant(1)) : builder.ISub(builder.Constant(waveSize - 1), lane);
            auto* selected = context.construct<LLVMBC::Argument>(word, 1, false);
            fixture.scalar.Bind(*selected, selector);
            arguments.push_back(selected);
        }
    }
    auto* signature = context.construct<LLVMBC::FunctionType>(context, resultType, std::move(parameters));
    fixture.source.Module().add_value_name(50, indexed ? IndexedLaneName(bits, floating) : dxil ? FirstLaneName(bits, floating) : UniformName(bits));
    auto* function = context.construct<LLVMBC::Function>(signature, 50, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    auto& result = fixture.scalar.Get(*call);
    auto write = [&](IrValue& value) {
        auto& offset = builder.ShiftLeftLogical(index, builder.Constant(bits == 64 ? 3 : 2));
        auto& lower = bits == 64 ? builder.CompositeExtract(value, 0) : value;
        fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &lower);
        if (bits == 64) fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(offset, builder.Constant(4)), 2, &builder.CompositeExtract(value, 1));
    };
    auto& encoded = bits == 1 ? builder.Select(result, builder.Constant(1), builder.Constant(0)) : floating ? builder.BitCastU32(result) : result;
    write(encoded);
    if (merge) {
        auto jump = [&](IrBlock& block) {
            builder.Branch(*merge);
            auto& term = program.Metadata().blockInfo[block.Id()].terminator;
            term.kind = TerminatorKind::Branch;
            term.trueBlock = merge->Id();
        };
        jump(*active);
        builder.SetInsertionPoint(*inactive);
        auto& marker = bits == 64 ? builder.ConstantU64(0xCAFEBABEDEADBEEF) : builder.Constant(0xDEADBEEF);
        write(marker);
        jump(*inactive);
        builder.SetInsertionPoint(*merge);
        program.Metadata().blockInfo[merge->Id()].terminator.kind = TerminatorKind::Return;
    } else program.Metadata().blockInfo[0].terminator.kind = TerminatorKind::Return;
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(program);
}
}
