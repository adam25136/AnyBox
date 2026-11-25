#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram ReadLaneKernel(unsigned wave, std::span<const std::uint32_t> selectors, unsigned selectorMode = 0) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    fixture.program.SetWaveSize(wave);
    auto& global = fixture.GlobalIndex();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto bind = [&](IrValue& value, unsigned index) {
        auto* argument = context.construct<LLVMBC::Argument>(word, index, false);
        fixture.scalar.Bind(*argument, value);
        return argument;
    };
    auto* payload = bind(builder.IAdd(builder.IMul(global, builder.Constant(17)), builder.Constant(5)), 0);
    auto* signature = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{word, word, word});
    fixture.source.Module().add_value_name(51, "xbox.ReadLane.i32");
    auto* function = context.construct<LLVMBC::Function>(signature, 51, fixture.source.Module());
    for (unsigned item = 0; item < selectors.size(); ++item) {
        LLVMBC::Value* selector = LLVMBC::ConstantInt::get(word, selectors[item]);
        if (selectorMode == 1) {
            auto& group = builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::WorkgroupId)), &builder.Constant(0)});
            selector = bind(builder.IAdd(builder.IMul(group, builder.Constant(33)), builder.Constant(selectors[item])), item + 1);
        }
        if (selectorMode == 2) selector = bind(global, item + 1);
        auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 51), payload, selector});
        auto& result = fixture.scalar.Get(*call);
        auto& offset = builder.IMul(builder.IAdd(builder.IMul(global, builder.Constant(static_cast<std::uint32_t>(selectors.size()))), builder.Constant(item)), builder.Constant(4));
        fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &result);
    }
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}
