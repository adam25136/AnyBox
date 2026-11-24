#pragma once

#include "../resources/Fixture.hpp"
#include "lowering/Buffer.hpp"
#include "lowering/Scalar.hpp"
#include "lowering/Function.hpp"
#include <optional>

namespace BufferTests {
class Bindings : public AnyBox::Shader::Dxil::IResourceBindings {
public:
    AnyBox::Shader::Dxil::BufferAddress ResolveBuffer(const AnyBox::Shader::BufferBinding&, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) override {
        if (!index.HasImmediate() || index.ImmediateU32() != 4 || nonUniform) throw std::runtime_error("Unexpected test buffer binding selection");
        return {builder.ConstantU64(0x100000000), builder.ConstantU64(64)};
    }
};

class Fixture {
public:
    explicit Fixture(bool structured, unsigned category = 0) : structured(structured), builder(program), scalar(program, builder) {
        source.Set(category, source.Buffer(category, structured ? 32 : 0));
        layout.emplace(source.Module());
        buffers.emplace(program, builder, *layout, bindings);
        scalar.SetBuffers(*buffers);
        auto& block = program.CreateBlock();
        program.SetEntryBlock(block);
        program.BlockOrder().push_back(&block);
        program.Metadata().blockInfo.push_back({});
        builder.SetInsertionPoint(block);
        handle = source.Handle(category, 4);
        scalar.Emit(*handle);
        instructions.push_back(handle);
    }
    LLVMBC::CallInst* Load(unsigned bits, unsigned index, unsigned offset, unsigned mask = 1) {
        auto& context = source.Context();
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* byte = LLVMBC::Type::getInt8Ty(context);
        auto* element = LLVMBC::Type::getIntTy(context, bits);
        auto* result = LLVMBC::StructType::get(context, {element, element, element, element, word}, "result");
        auto* signature = context.construct<LLVMBC::FunctionType>(context, result, LLVMBC::Vector<LLVMBC::Type*>{word, handle->getType(), word, word, byte, word});
        auto identifier = "dx.op.rawBufferLoad.i" + std::to_string(bits);
        source.Module().add_value_name(bits, LLVMBC::String(identifier.begin(), identifier.end()));
        auto* declaration = context.construct<LLVMBC::Function>(signature, bits, source.Module());
        auto* location = structured ? static_cast<LLVMBC::Value*>(LLVMBC::ConstantInt::get(word, offset)) : LLVMBC::UndefValue::get(word);
        auto* call = context.construct<LLVMBC::CallInst>(signature, declaration, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 139), handle, LLVMBC::ConstantInt::get(word, index), location, LLVMBC::ConstantInt::get(byte, mask), LLVMBC::ConstantInt::get(word, bits / 8)});
        scalar.Emit(*call);
        instructions.push_back(call);
        return call;
    }
    LLVMBC::CallInst* TypedLoad(unsigned format, unsigned offset) {
        auto& context = source.Context();
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* result = LLVMBC::StructType::get(context, {word, word, word, word, word}, "typedResult");
        auto* signature = context.construct<LLVMBC::FunctionType>(context, result, LLVMBC::Vector<LLVMBC::Type*>{word, handle->getType(), word, word});
        source.Module().add_value_name(512, "xbox.TypedLoad.i32");
        auto* declaration = context.construct<LLVMBC::Function>(signature, 512, source.Module());
        auto* call = context.construct<LLVMBC::CallInst>(signature, declaration, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 138), handle, LLVMBC::ConstantInt::get(word, offset), LLVMBC::ConstantInt::get(word, format)});
        scalar.Emit(*call);
        instructions.push_back(call);
        return call;
    }
    ShaderRecompiler::IrValue& Extract(LLVMBC::CallInst* load, unsigned index) {
        auto* type = load->getType()->getStructElementType(index);
        auto* instruction = source.Context().construct<LLVMBC::ExtractValueInst>(type, load, LLVMBC::Vector<unsigned>{index});
        auto& value = scalar.Get(*instruction);
        instructions.push_back(instruction);
        return value;
    }
    ShaderRecompiler::IrProgram LowerFunction() {
        auto& context = source.Context();
        auto* signature = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
        auto* function = context.construct<LLVMBC::Function>(signature, 1024, source.Module());
        auto* block = context.construct<LLVMBC::BasicBlock>(context);
        for (auto* instruction : instructions) block->add_instruction(instruction);
        block->add_instruction(context.construct<LLVMBC::ReturnInst>(nullptr));
        function->set_basic_blocks({block});
        ShaderRecompiler::IrProgram output;
        AnyBox::Shader::Dxil::FunctionLowering(output, *layout, bindings).Run(*function);
        return output;
    }
    ShaderRecompiler::IrProgram& Finish() {
        (void)builder.Emit(ShaderRecompiler::IrOpcode::Return, ShaderRecompiler::IrType::Void, {});
        ShaderRecompiler::ValidateProgram(program, true);
        return program;
    }
    void WriteOutput(ShaderRecompiler::IrValue& value) {
        using namespace ShaderRecompiler;
        auto& low = builder.Constant(0);
        auto& high = builder.Constant(2);
        auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
        MemoryInfo memory;
        memory.kind = ResourceKind::Global;
        memory.addressIsFull = true;
        MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
        program.Resources().memoryInfo.push_back(memory);
        (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &low, &high, &value, &builder.ConstantBool(true)}, flags);
    }

private:
    ResourceTests::Fixture source;
    bool structured;
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::IrBuilder builder;
    AnyBox::Shader::Dxil::ScalarLowering scalar;
    Bindings bindings;
    std::optional<AnyBox::Shader::Dxil::ResourceLayout> layout;
    std::optional<AnyBox::Shader::Dxil::BufferLowering> buffers;
    LLVMBC::CallInst* handle;
    std::vector<LLVMBC::Instruction*> instructions;
};

std::uint64_t Evaluate(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrValue& output);
void CheckSpirv();
}
