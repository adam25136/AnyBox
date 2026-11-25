#pragma once

#include "../resources/Fixture.hpp"
#include "lowering/Function.hpp"

namespace BufferTests {
class HalfMemoryBindings : public AnyBox::Shader::Dxil::IResourceBindings {
public:
    AnyBox::Shader::Dxil::BufferAddress ResolveBuffer(const AnyBox::Shader::BufferBinding&, ShaderRecompiler::IrValue&, bool, ShaderRecompiler::IrBuilder& builder) override {
        return {builder.ConstantU64(0x200000000), builder.ConstantU64(128 * 12)};
    }
};

class HalfMemoryBase {
public:
    explicit HalfMemoryBase(bool floating) {
        auto& context = source.Context();
        word = LLVMBC::Type::getInt32Ty(context);
        element = floating ? LLVMBC::Type::getHalfTy(context) : LLVMBC::Type::getInt16Ty(context);
        entry = block = context.construct<LLVMBC::BasicBlock>(context);
        blocks.push_back(entry);
        source.Set(1, source.Buffer(1, 0));
        output = source.Handle(1, 4);
        block->add_instruction(output);
        global = Call("dx.op.threadId.i32", word, {Integer(93), Integer(0)});
        local = Call("dx.op.threadIdInGroup.i32", word, {Integer(95), Integer(0)});
    }
    template<class TInstruction, class... TArguments>
    TInstruction* Emit(TArguments&&... arguments) {
        auto* instruction = source.Context().construct<TInstruction>(std::forward<TArguments>(arguments)...);
        block->add_instruction(instruction);
        return instruction;
    }
    LLVMBC::ConstantInt* Integer(std::uint32_t value) { return LLVMBC::ConstantInt::get(word, value); }
    LLVMBC::Value* Value(std::uint32_t bits) {
        if (element->getTypeID() == LLVMBC::Type::TypeID::HalfTyID) return LLVMBC::ConstantFP::get(element, bits);
        return LLVMBC::ConstantInt::get(element, bits);
    }
    LLVMBC::Value* Bits(LLVMBC::Value* value) {
        auto* shortType = LLVMBC::Type::getInt16Ty(source.Context());
        if (value->getType()->getTypeID() == LLVMBC::Type::TypeID::HalfTyID) value = Emit<LLVMBC::CastInst>(shortType, value, LLVMBC::Instruction::BitCast);
        return Emit<LLVMBC::CastInst>(word, value, LLVMBC::Instruction::ZExt);
    }
    LLVMBC::Value* Narrow(LLVMBC::Value* value) {
        auto* shortType = LLVMBC::Type::getInt16Ty(source.Context());
        auto* integer = Emit<LLVMBC::CastInst>(shortType, value, LLVMBC::Instruction::Trunc);
        return element == shortType ? static_cast<LLVMBC::Value*>(integer) : Emit<LLVMBC::CastInst>(element, integer, LLVMBC::Instruction::BitCast);
    }
    LLVMBC::Value* Binary(LLVMBC::Value* left, LLVMBC::Value* right, LLVMBC::Instruction::BinaryOps operation) { return Emit<LLVMBC::BinaryOperator>(left, right, operation); }
    LLVMBC::Value* Pointer(LLVMBC::Value* base, LLVMBC::Value* index) {
        auto space = base->getType()->getPointerAddressSpace();
        auto* type = LLVMBC::PointerType::get(element, space);
        return Emit<LLVMBC::GetElementPtrInst>(type, LLVMBC::Vector<LLVMBC::Value*>{base, Integer(0), index}, true);
    }
    LLVMBC::CallInst* Call(const std::string& name, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        auto& context = source.Context();
        source.Module().add_value_name(nextId, LLVMBC::String(name.begin(), name.end()));
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* type = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* callee = context.construct<LLVMBC::Function>(type, nextId++, source.Module());
        return Emit<LLVMBC::CallInst>(type, callee, std::move(arguments));
    }
    void Output(unsigned component, LLVMBC::Value* value) {
        auto* offset = Binary(Binary(global, Integer(12), LLVMBC::Instruction::Mul), Integer(component * 4), LLVMBC::Instruction::Add);
        auto* unused = LLVMBC::UndefValue::get(word);
        auto* mask = LLVMBC::ConstantInt::get(LLVMBC::Type::getInt8Ty(source.Context()), 1);
        Call("dx.op.rawBufferStore.i32", LLVMBC::Type::getVoidTy(source.Context()), {Integer(140), output, offset, unused, Bits(value), unused, unused, unused, mask, Integer(4)});
    }
    ShaderRecompiler::IrProgram Lower(unsigned wave) {
        Emit<LLVMBC::ReturnInst>(nullptr);
        auto& context = source.Context();
        auto* type = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
        auto* function = context.construct<LLVMBC::Function>(type, 1024, source.Module());
        function->set_basic_blocks(blocks);
        AnyBox::Shader::Dxil::ResourceLayout resources(source.Module());
        HalfMemoryBindings bindings;
        ShaderRecompiler::IrProgram program;
        program.SetWaveSize(wave);
        AnyBox::Shader::Dxil::FunctionLowering(program, resources, bindings).Run(*function);
        return program;
    }
    ResourceTests::Fixture source;
    LLVMBC::Type* word;
    LLVMBC::Type* element;
    LLVMBC::BasicBlock* entry;
    LLVMBC::BasicBlock* block;
    LLVMBC::Value* global;
    LLVMBC::Value* local;
    LLVMBC::CallInst* output;
    LLVMBC::Vector<LLVMBC::BasicBlock*> blocks;

private:
    unsigned nextId = 10;
};
}
