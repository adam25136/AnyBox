#pragma once

#include "../resources/Fixture.hpp"
#include "lowering/Scalar.hpp"

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeFloatCallKernel(unsigned opcode, unsigned operands, const char* intrinsic = nullptr) {
    using namespace ShaderRecompiler;
    if (operands != 2 && operands != 3) throw std::invalid_argument("Floating kernel requires two or three operands");
    ResourceTests::Fixture source;
    auto& context = source.Context();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto integer = [&](unsigned value) { return LLVMBC::ConstantInt::get(word, value); };
    auto call = [&](const char* name, unsigned id, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        source.Module().add_value_name(id, name);
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* signature = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* function = context.construct<LLVMBC::Function>(signature, id, source.Module());
        return context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    };
    IrProgram program;
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    program.BlockOrder() = {&block};
    program.Metadata().blockInfo.resize(1);
    IrBuilder builder(program);
    builder.SetInsertionPoint(block);
    AnyBox::Shader::Dxil::ScalarLowering scalar(program, builder);
    auto& index = scalar.Get(*call("dx.op.threadId.i32", 93, word, {integer(93), integer(0)}));
    auto load = [&](IrValue& element) -> IrValue& {
        auto& low = builder.ShiftLeftLogical(element, builder.Constant(2));
        auto& high = builder.Constant(1);
        auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
        MemoryInfo memory;
        memory.kind = ResourceKind::Global;
        memory.addressIsFull = true;
        MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
        program.Resources().memoryInfo.push_back(memory);
        auto& bits = builder.Emit(IrOpcode::LoadAddressU32, IrType::U32, {&resource, &low, &high, &builder.ConstantBool(true)}, flags);
        return builder.BitCastF32(bits);
    };
    auto* floating = LLVMBC::Type::getFloatTy(context);
    LLVMBC::Vector<LLVMBC::Value*> arguments{integer(opcode)};
    for (unsigned operand = 0; operand < operands; ++operand) {
        auto& shifted = builder.ShiftRightLogical(index, builder.Constant((operands - operand - 1) * 4));
        auto& element = builder.BitwiseAnd(shifted, builder.Constant(15));
        auto* argument = context.construct<LLVMBC::Argument>(floating, operand, false);
        scalar.Bind(*argument, load(element));
        arguments.push_back(argument);
    }
    auto* identifier = intrinsic ? intrinsic : operands == 2 ? "dx.op.binary.f32" : "dx.op.tertiary.f32";
    auto& result = scalar.Get(*call(identifier, 100, floating, std::move(arguments)));
    auto& nan = builder.Emit(IrOpcode::FPIsNan32, IrType::Bool, {&result});
    auto& bits = builder.Select(nan, builder.Constant(0x7FC00000U), builder.BitCastU32(result));
    auto& low = builder.ShiftLeftLogical(index, builder.Constant(2));
    auto& high = builder.Constant(2);
    auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
    program.Resources().memoryInfo.push_back(memory);
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &low, &high, &bits, &builder.ConstantBool(true)}, flags);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return program;
}
}
