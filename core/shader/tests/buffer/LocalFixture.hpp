#pragma once

#include "../resources/Fixture.hpp"
#include "lowering/Function.hpp"
#include <bit>

namespace AnyBox::Vulkan { class Context; }

namespace BufferTests {
class LocalBindings : public AnyBox::Shader::Dxil::IResourceBindings {
public:
    AnyBox::Shader::Dxil::BufferAddress ResolveBuffer(const AnyBox::Shader::BufferBinding&, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) override {
        if (!index.HasImmediate() || index.ImmediateU32() != 4 || nonUniform) throw std::runtime_error("Unexpected local test resource selection");
        return {builder.ConstantU64(0x200000000), builder.ConstantU64(64 * 12)};
    }
};

class LocalFixture {
public:
    explicit LocalFixture(std::optional<std::int32_t> constantIndex = {}, bool invalidDynamic = false, unsigned pointerMode = 0) {
        auto& context = source.Context();
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* floating = LLVMBC::Type::getFloatTy(context);
        block = context.construct<LLVMBC::BasicBlock>(context);
        source.Set(1, source.Buffer(1, 0));
        auto* handle = source.Handle(1, 4);
        block->add_instruction(handle);
        auto* thread = call("dx.op.threadId.i32", 93, word, {integer(93), integer(0)});
        auto* tableType = LLVMBC::ArrayType::get(floating, 576);
        auto* privateType = LLVMBC::ArrayType::get(word, 4);
        auto* table = emit<LLVMBC::AllocaInst>(LLVMBC::PointerType::get(tableType, 0), tableType, integer(1));
        auto* local = emit<LLVMBC::AllocaInst>(LLVMBC::PointerType::get(privateType, 0), privateType, integer(1));
        if (!constantIndex && !invalidDynamic) {
            for (unsigned index = 0; index < 576; ++index) {
                auto* pointer = element(table, floating, integer(index));
                LLVMBC::Value* value = LLVMBC::ConstantFP::get(floating, std::bit_cast<std::uint32_t>(index * 0.25f));
                if (pointerMode != 0) {
                    pointer = emit<LLVMBC::CastInst>(LLVMBC::PointerType::get(word, pointerMode == 2 ? 3u : 0u), pointer, LLVMBC::Instruction::BitCast);
                    value = integer(std::bit_cast<std::uint32_t>(index * 0.25f));
                }
                (void)emit<LLVMBC::StoreInst>(pointer, value);
            }
        }
        for (unsigned index = 0; index < 4; ++index) {
            auto* value = binary(binary(thread, integer(17), LLVMBC::Instruction::Mul), integer(index), LLVMBC::Instruction::Add);
            (void)emit<LLVMBC::StoreInst>(element(local, word, integer(index)), value);
        }
        LLVMBC::Value* selected = nullptr;
        if (constantIndex) selected = integer(static_cast<std::uint32_t>(*constantIndex));
        else if (invalidDynamic) selected = binary(thread, integer(576), LLVMBC::Instruction::Add);
        else selected = binary(binary(thread, integer(37), LLVMBC::Instruction::Mul), integer(576), LLVMBC::Instruction::URem);
        auto* coefficient = emit<LLVMBC::LoadInst>(floating, element(table, floating, selected));
        auto* bits = emit<LLVMBC::CastInst>(word, coefficient, LLVMBC::Instruction::BitCast);
        auto* localValue = emit<LLVMBC::LoadInst>(word, element(local, word, binary(thread, integer(3), LLVMBC::Instruction::And)));
        auto* byteOffset = binary(thread, integer(12), LLVMBC::Instruction::Mul);
        store(handle, byteOffset, bits);
        store(handle, binary(byteOffset, integer(4), LLVMBC::Instruction::Add), localValue);
        store(handle, binary(byteOffset, integer(8), LLVMBC::Instruction::Add), thread);
        (void)emit<LLVMBC::ReturnInst>(nullptr);
    }
    ShaderRecompiler::IrProgram Lower() {
        auto& context = source.Context();
        auto* type = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
        auto* function = context.construct<LLVMBC::Function>(type, 1024, source.Module());
        function->set_basic_blocks({block});
        AnyBox::Shader::Dxil::ResourceLayout resources(source.Module());
        ShaderRecompiler::IrProgram program;
        AnyBox::Shader::Dxil::FunctionLowering(program, resources, bindings).Run(*function);
        return program;
    }

private:
    LLVMBC::ConstantInt* integer(std::uint32_t value) { return LLVMBC::ConstantInt::get(LLVMBC::Type::getInt32Ty(source.Context()), value); }
    template<class TInstruction, class... TArguments>
    TInstruction* emit(TArguments&&... arguments) {
        auto* instruction = source.Context().construct<TInstruction>(std::forward<TArguments>(arguments)...);
        block->add_instruction(instruction);
        return instruction;
    }
    LLVMBC::Value* binary(LLVMBC::Value* left, LLVMBC::Value* right, LLVMBC::Instruction::BinaryOps operation) { return emit<LLVMBC::BinaryOperator>(left, right, operation); }
    LLVMBC::Value* element(LLVMBC::Value* pointer, LLVMBC::Type* type, LLVMBC::Value* index) {
        return emit<LLVMBC::GetElementPtrInst>(LLVMBC::PointerType::get(type, 0), LLVMBC::Vector<LLVMBC::Value*>{pointer, integer(0), index}, true);
    }
    LLVMBC::CallInst* call(const std::string& name, unsigned id, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        auto& context = source.Context();
        auto& module = source.Module();
        module.add_value_name(id, LLVMBC::String(name.begin(), name.end()));
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* type = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* function = context.construct<LLVMBC::Function>(type, id, module);
        return emit<LLVMBC::CallInst>(type, function, std::move(arguments));
    }
    void store(LLVMBC::Value* handle, LLVMBC::Value* offset, LLVMBC::Value* value) {
        auto* word = LLVMBC::Type::getInt32Ty(source.Context());
        auto* unused = LLVMBC::UndefValue::get(word);
        auto* mask = LLVMBC::ConstantInt::get(LLVMBC::Type::getInt8Ty(source.Context()), 1);
        (void)call("dx.op.rawBufferStore.i32", 140, LLVMBC::Type::getVoidTy(source.Context()), {integer(140), handle, offset, unused, value, unused, unused, unused, mask, integer(4)});
    }
    ResourceTests::Fixture source;
    LocalBindings bindings;
    LLVMBC::BasicBlock* block;
};

void CheckLocals(AnyBox::Vulkan::Context& context);
}
