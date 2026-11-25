#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram ConditionalMaskKernel(unsigned wave, std::span<const std::uint64_t> masks, unsigned representation = 0, bool dynamic = false) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    fixture.program.SetWaveSize(wave);
    auto* word = LLVMBC::Type::getInt32Ty(context);
    LLVMBC::Type* pair = representation < 3 ? static_cast<LLVMBC::Type*>(LLVMBC::StructType::get(context, {word, word}, "pair")) : LLVMBC::VectorType::get(2, word);
    auto bind = [&](IrValue& input, unsigned id) {
        auto* argument = context.construct<LLVMBC::Argument>(word, id, false);
        fixture.scalar.Bind(*argument, input);
        return argument;
    };
    auto integer = [&](std::uint32_t value) { return LLVMBC::ConstantInt::get(word, value); };
    auto& global = fixture.GlobalIndex();
    auto* low = bind(builder.IAdd(builder.IMul(global, builder.Constant(17)), builder.Constant(5)), 0);
    auto* high = bind(builder.BitwiseXor(global, builder.Constant(0xA55A8420)), 1);
    auto* values = context.construct<LLVMBC::CompositeConstructInst>(pair, LLVMBC::Vector<LLVMBC::Value*>{low, high});
    auto* signature = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{word, pair, pair});
    fixture.source.Module().add_value_name(170, "xbox.V_CNDMASK_B32");
    auto* function = context.construct<LLVMBC::Function>(signature, 170, fixture.source.Module());
    for (unsigned item = 0; item < masks.size(); ++item) {
        LLVMBC::Vector<LLVMBC::Value*> elements{integer(static_cast<std::uint32_t>(masks[item])), integer(static_cast<std::uint32_t>(masks[item] >> 32))};
        if (dynamic) {
            elements[0] = bind(builder.BitwiseXor(global, builder.Constant(static_cast<std::uint32_t>(masks[item]))), 2 + item * 2);
            elements[1] = bind(builder.BitwiseXor(global, builder.Constant(static_cast<std::uint32_t>(masks[item] >> 32))), 3 + item * 2);
        }
        LLVMBC::Value* mask;
        if (representation == 6) mask = context.construct<LLVMBC::ConstantAggregateZero>(pair);
        else if (dynamic || representation == 2 || representation == 5) mask = context.construct<LLVMBC::CompositeConstructInst>(pair, std::move(elements));
        else if (representation == 0) mask = context.construct<LLVMBC::ConstantDataArray>(pair, std::move(elements));
        else if (representation == 3) mask = context.construct<LLVMBC::ConstantDataVector>(pair, std::move(elements));
        else mask = context.construct<LLVMBC::ConstantAggregate>(pair, std::move(elements));
        auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{integer(170), mask, values});
        auto& result = fixture.scalar.Get(*call);
        auto& offset = builder.IMul(builder.IAdd(builder.IMul(global, builder.Constant(static_cast<std::uint32_t>(masks.size()))), builder.Constant(item)), builder.Constant(4));
        fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &result);
    }
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}
