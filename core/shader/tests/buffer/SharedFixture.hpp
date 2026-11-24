#pragma once

#include "../resources/Fixture.hpp"
#include "lowering/Function.hpp"
#include <optional>

namespace BufferTests {
class SharedBindings : public AnyBox::Shader::Dxil::IResourceBindings {
public:
    AnyBox::Shader::Dxil::BufferAddress ResolveBuffer(const AnyBox::Shader::BufferBinding&, ShaderRecompiler::IrValue&, bool, ShaderRecompiler::IrBuilder& builder) override {
        return {builder.ConstantU64(0x200000000), builder.ConstantU64(128 * 12)};
    }
};

class SharedFixture {
public:
    explicit SharedFixture(std::optional<std::int32_t> constantRead = {}, bool invalidDynamic = false, unsigned space = 3) {
        auto& context = source.Context();
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* floating = LLVMBC::Type::getFloatTy(context);
        auto* firstType = LLVMBC::ArrayType::get(floating, 64);
        auto* secondType = LLVMBC::ArrayType::get(LLVMBC::ArrayType::get(word, 32), 2);
        auto* first = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(firstType, space), LLVMBC::GlobalVariable::InternalLinkage, false);
        firstGlobal = first;
        auto* second = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(secondType, 3), LLVMBC::GlobalVariable::InternalLinkage, false);
        first->set_initializer(LLVMBC::UndefValue::get(firstType));
        second->set_initializer(LLVMBC::UndefValue::get(secondType));
        source.Module().add_global_variable(first);
        source.Module().add_global_variable(second);
        source.Set(1, source.Buffer(1, 0));
        block = context.construct<LLVMBC::BasicBlock>(context);
        auto* output = source.Handle(1, 4);
        block->add_instruction(output);
        auto* global = call("dx.op.threadId.i32", 93, word, {integer(93), integer(0)});
        auto* local = call("dx.op.threadIdInGroup.i32", 95, word, {integer(95), integer(0)});
        auto* value = binary(binary(global, integer(17), LLVMBC::Instruction::Mul), integer(5), LLVMBC::Instruction::Add);
        auto* floatValue = emit<LLVMBC::CastInst>(floating, value, LLVMBC::Instruction::BitCast);
        emit<LLVMBC::StoreInst>(element(first, floating, 3, {integer(0), local}), floatValue);
        auto* row = binary(local, integer(5), LLVMBC::Instruction::LShr);
        auto* column = binary(local, integer(31), LLVMBC::Instruction::And);
        auto* secondValue = binary(value, integer(0xA5A5A5A5), LLVMBC::Instruction::Xor);
        emit<LLVMBC::StoreInst>(element(second, word, 3, {integer(0), row, column}), secondValue);
        auto* privateType = LLVMBC::ArrayType::get(word, 1);
        auto* scratch = emit<LLVMBC::AllocaInst>(LLVMBC::PointerType::get(privateType, 0), privateType, integer(1));
        auto* privatePointer = element(scratch, word, 0, {integer(0), integer(0)});
        emit<LLVMBC::StoreInst>(privatePointer, binary(global, integer(7), LLVMBC::Instruction::Add));
        call("dx.op.barrier", 80, LLVMBC::Type::getVoidTy(context), {integer(80), integer(9)});
        LLVMBC::Value* selected = constantRead ? static_cast<LLVMBC::Value*>(integer(static_cast<std::uint32_t>(*constantRead))) : binary(local, integer(invalidDynamic ? 64 : 32), invalidDynamic ? LLVMBC::Instruction::Add : LLVMBC::Instruction::Xor);
        auto* firstLoad = emit<LLVMBC::LoadInst>(floating, element(first, floating, 3, {integer(0), selected}));
        auto* firstBits = emit<LLVMBC::CastInst>(word, firstLoad, LLVMBC::Instruction::BitCast);
        auto* reverse = binary(integer(63), local, LLVMBC::Instruction::Sub);
        auto* reverseRow = binary(reverse, integer(5), LLVMBC::Instruction::LShr);
        auto* reverseColumn = binary(reverse, integer(31), LLVMBC::Instruction::And);
        auto* secondLoad = emit<LLVMBC::LoadInst>(word, element(second, word, 3, {integer(0), reverseRow, reverseColumn}));
        auto* privateLoad = emit<LLVMBC::LoadInst>(word, privatePointer);
        auto* offset = binary(global, integer(12), LLVMBC::Instruction::Mul);
        store(output, offset, firstBits);
        store(output, binary(offset, integer(4), LLVMBC::Instruction::Add), secondLoad);
        store(output, binary(offset, integer(8), LLVMBC::Instruction::Add), privateLoad);
        emit<LLVMBC::ReturnInst>(nullptr);
    }
    ShaderRecompiler::IrProgram Lower() {
        auto& context = source.Context();
        auto* type = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
        auto* function = context.construct<LLVMBC::Function>(type, 1024, source.Module());
        function->set_basic_blocks({block});
        AnyBox::Shader::Dxil::ResourceLayout layout(source.Module());
        SharedBindings bindings;
        ShaderRecompiler::IrProgram program;
        AnyBox::Shader::Dxil::FunctionLowering(program, layout, bindings).Run(*function);
        return program;
    }
    void InitializeShared() {
        auto* type = firstGlobal->getType()->getPointerElementType();
        firstGlobal->set_initializer(source.Context().construct<LLVMBC::ConstantAggregateZero>(type));
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
    LLVMBC::Value* element(LLVMBC::Value* pointer, LLVMBC::Type* type, unsigned space, LLVMBC::Vector<LLVMBC::Value*> indices) {
        bool constant = pointer->get_value_kind() == LLVMBC::ValueKind::Global;
        for (auto* index : indices) constant &= index->get_value_kind() == LLVMBC::ValueKind::ConstantInt;
        indices.insert(indices.begin(), pointer);
        if (constant) return source.Context().construct<LLVMBC::ConstantExpr>(LLVMBC::Instruction::GetElementPtr, LLVMBC::PointerType::get(type, space), std::move(indices));
        return emit<LLVMBC::GetElementPtrInst>(LLVMBC::PointerType::get(type, space), std::move(indices), true);
    }
    LLVMBC::CallInst* call(const std::string& name, unsigned id, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        auto& context = source.Context();
        auto& module = source.Module();
        module.add_value_name(id, LLVMBC::String(name.begin(), name.end()));
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* signature = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* function = context.construct<LLVMBC::Function>(signature, id, module);
        return emit<LLVMBC::CallInst>(signature, function, std::move(arguments));
    }
    void store(LLVMBC::Value* handle, LLVMBC::Value* offset, LLVMBC::Value* value) {
        auto* word = LLVMBC::Type::getInt32Ty(source.Context());
        auto* unused = LLVMBC::UndefValue::get(word);
        auto* mask = LLVMBC::ConstantInt::get(LLVMBC::Type::getInt8Ty(source.Context()), 1);
        call("dx.op.rawBufferStore.i32", 140, LLVMBC::Type::getVoidTy(source.Context()), {integer(140), handle, offset, unused, value, unused, unused, unused, mask, integer(4)});
    }
    ResourceTests::Fixture source;
    LLVMBC::BasicBlock* block;
    LLVMBC::GlobalVariable* firstGlobal;
};
}
