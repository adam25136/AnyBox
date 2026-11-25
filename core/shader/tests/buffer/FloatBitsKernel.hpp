#pragma once

#include "AtomicKernel.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeFloatBitsKernel(unsigned mode, bool linear = false) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* floating = LLVMBC::Type::getFloatTy(context);
    LLVMBC::Vector<LLVMBC::Value*> arguments;
    if (mode != 0) arguments.push_back(LLVMBC::ConstantInt::get(word, mode));
    for (unsigned operand = 0; operand < (mode == 0 ? 2u : 1u); ++operand) {
        auto& base = operand == 0 ? builder.ShiftRightLogical(index, builder.Constant(4)) : index;
        auto& selected = linear ? index : builder.BitwiseAnd(base, builder.Constant(15));
        auto& offset = builder.ShiftLeftLogical(selected, builder.Constant(2));
        auto& value = builder.BitCastF32(fixture.Memory(IrOpcode::LoadAddressU32, offset, 1));
        auto* argument = context.construct<LLVMBC::Argument>(floating, operand, false);
        fixture.scalar.Bind(*argument, value);
        arguments.push_back(argument);
    }
    fixture.source.Module().add_value_name(6, mode == 0 ? "llvm.copysign.f32" : "dx.op.unary.f32");
    LLVMBC::Vector<LLVMBC::Type*> parameters;
    for (auto* argument : arguments) parameters.push_back(argument->getType());
    auto* signature = context.construct<LLVMBC::FunctionType>(context, floating, std::move(parameters));
    auto* function = context.construct<LLVMBC::Function>(signature, 6, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    auto& value = fixture.scalar.Get(*call);
    auto* bits = &builder.BitCastU32(value);
    if (mode == 22 || mode == 28) {
        auto& nan = builder.Emit(IrOpcode::FPIsNan32, IrType::Bool, {&value});
        bits = &builder.Select(nan, builder.Constant(0x7FC00000), *bits);
    }
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, bits);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}
