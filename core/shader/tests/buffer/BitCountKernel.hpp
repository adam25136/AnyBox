#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram BitCountKernel(unsigned wave, std::span<const std::uint64_t> masks, unsigned mode = 0, bool prefix = false) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    fixture.program.SetWaveSize(wave);
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* pair = LLVMBC::StructType::get(context, {word, word}, "mask");
    auto integer = [&](std::uint32_t value) { return LLVMBC::ConstantInt::get(word, value); };
    auto bind = [&](IrValue& value, unsigned id) {
        auto* argument = context.construct<LLVMBC::Argument>(word, id, false);
        fixture.scalar.Bind(*argument, value);
        return argument;
    };
    auto& global = fixture.GlobalIndex();
    auto& local = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::LocalInvocationIndex)), &builder.Constant(0)});
    auto& lane = builder.BitwiseAnd(local, builder.Constant(63));
    auto& shift = builder.BitwiseAnd(lane, builder.Constant(31));
    auto opcode = prefix ? 107u : 130u;
    fixture.source.Module().add_value_name(130, prefix ? "xbox.MBCNT64" : "xbox.S_BCNT1_U64");
    fixture.source.Module().add_value_name(25, "xbox.Ballot64");
    auto* signature = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{word, pair});
    auto* function = context.construct<LLVMBC::Function>(signature, 130, fixture.source.Module());
    auto* ballotSignature = context.construct<LLVMBC::FunctionType>(context, pair, LLVMBC::Vector<LLVMBC::Type*>{word, word});
    auto* ballotFunction = context.construct<LLVMBC::Function>(ballotSignature, 25, fixture.source.Module());
    for (unsigned item = 0; item < masks.size(); ++item) {
        auto low = static_cast<std::uint32_t>(masks[item]);
        auto high = static_cast<std::uint32_t>(masks[item] >> 32);
        LLVMBC::Value* input;
        if (mode == 0) input = context.construct<LLVMBC::ConstantDataArray>(pair, LLVMBC::Vector<LLVMBC::Value*>{integer(low), integer(high)});
        else if (mode == 1) {
            auto* lower = bind(builder.BitwiseXor(global, builder.Constant(low)), 2 * item);
            auto* upper = bind(builder.BitwiseXor(global, builder.Constant(high)), 2 * item + 1);
            input = context.construct<LLVMBC::CompositeConstructInst>(pair, LLVMBC::Vector<LLVMBC::Value*>{lower, upper});
        } else {
            auto& selected = builder.Select(builder.ULessThan(lane, builder.Constant(32)), builder.Constant(low), builder.Constant(high));
            auto& predicate = builder.BitwiseAnd(builder.ShiftRightLogical(selected, shift), builder.Constant(1));
            auto* argument = bind(predicate, item);
            input = context.construct<LLVMBC::CallInst>(ballotSignature, ballotFunction, LLVMBC::Vector<LLVMBC::Value*>{integer(25), argument});
        }
        auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{integer(opcode), input});
        auto& count = fixture.scalar.Get(*call);
        auto& offset = builder.IMul(builder.IAdd(builder.IMul(global, builder.Constant(static_cast<std::uint32_t>(masks.size()))), builder.Constant(item)), builder.Constant(4));
        fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &count);
    }
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}
