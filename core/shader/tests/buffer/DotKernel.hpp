#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeDotKernel(unsigned dimensions, bool precise) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* floating = LLVMBC::Type::getFloatTy(context);
    LLVMBC::Vector<LLVMBC::Value*> arguments{LLVMBC::ConstantInt::get(word, 52 + dimensions)};
    for (unsigned side = 0; side < 2; ++side) {
        for (unsigned part = 0; part < dimensions; ++part) {
            auto& base = side == 0 ? builder.ShiftRightLogical(index, builder.Constant(4)) : index;
            auto& selected = builder.BitwiseAnd(builder.IAdd(base, builder.Constant(part * (side == 0 ? 1 : 3))), builder.Constant(15));
            auto& offset = builder.ShiftLeftLogical(selected, builder.Constant(2));
            auto& value = builder.BitCastF32(fixture.Memory(IrOpcode::LoadAddressU32, offset, 1));
            auto* argument = context.construct<LLVMBC::Argument>(floating, side * dimensions + part, false);
            fixture.scalar.Bind(*argument, value);
            arguments.push_back(argument);
        }
    }
    fixture.source.Module().add_value_name(54, dimensions == 2 ? "dx.op.dot2.f32" : dimensions == 3 ? "dx.op.dot3.f32" : "dx.op.dot4.f32");
    LLVMBC::Vector<LLVMBC::Type*> parameters;
    for (auto* argument : arguments) parameters.push_back(argument->getType());
    auto* signature = context.construct<LLVMBC::FunctionType>(context, floating, std::move(parameters));
    auto* function = context.construct<LLVMBC::Function>(signature, 54, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    if (precise) call->setMetadata("dx.precise", fixture.source.Node({fixture.source.Integer(1)}));
    auto& value = fixture.scalar.Get(*call);
    auto& nan = builder.Emit(IrOpcode::FPIsNan32, IrType::Bool, {&value});
    auto& bits = builder.Select(nan, builder.Constant(0x7FC00000), builder.BitCastU32(value));
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &bits);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}
