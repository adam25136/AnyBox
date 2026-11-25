#pragma once

#include "GlobalWords.hpp"
#include "../resources/Fixture.hpp"
#include "lowering/Function.hpp"
#include <optional>

namespace BufferTests {
class PrivateGlobalBindings : public AnyBox::Shader::Dxil::IResourceBindings {
public:
    AnyBox::Shader::Dxil::BufferAddress ResolveBuffer(const AnyBox::Shader::BufferBinding&, ShaderRecompiler::IrValue&, bool, ShaderRecompiler::IrBuilder& builder) override {
        return {builder.ConstantU64(0x200000000), builder.ConstantU64(128 * 12)};
    }
};

class PrivateGlobalFixture {
public:
    PrivateGlobalFixture(unsigned mode, bool expressions, std::optional<std::int32_t> constantIndex = {}, bool outside = false) {
        auto& context = source.Context();
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* element = mode == 2 ? LLVMBC::Type::getFloatTy(context) : mode == 3 ? LLVMBC::Type::getInt64Ty(context) : word;
        auto* row = LLVMBC::ArrayType::get(element, 4);
        auto* tableType = LLVMBC::ArrayType::get(row, 2);
        LLVMBC::Vector<LLVMBC::Value*> rows;
        for (unsigned group = 0; group < 2; ++group) {
            LLVMBC::Vector<LLVMBC::Value*> values;
            for (unsigned index = 0; index < 4; ++index) {
                auto value = ConstantWords[group * 4 + index];
                auto wide = (static_cast<std::uint64_t>(value ^ 0xA55A8420) << 32) | value;
                values.push_back(mode == 2 ? static_cast<LLVMBC::Value*>(LLVMBC::ConstantFP::get(element, value)) : LLVMBC::ConstantInt::get(element, mode == 3 ? wide : value));
            }
            rows.push_back(context.construct<LLVMBC::ConstantAggregate>(row, std::move(values)));
        }
        auto* table = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(tableType, mode == 8 ? 2u : 0u), mode == 7 ? LLVMBC::GlobalVariable::ExternalLinkage : LLVMBC::GlobalVariable::InternalLinkage, false);
        LLVMBC::Constant* initializer = context.construct<LLVMBC::ConstantAggregate>(tableType, std::move(rows));
        if (mode == 0) initializer = context.construct<LLVMBC::ConstantAggregateZero>(tableType);
        if (mode == 4) initializer = LLVMBC::UndefValue::get(tableType);
        if (mode == 6) initializer = integer(0);
        if (mode == 9) initializer = LLVMBC::UndefValue::get(word);
        if (mode != 5) table->set_initializer(initializer);
        auto* guard = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(word, 0), LLVMBC::GlobalVariable::InternalLinkage, false);
        guard->set_initializer(integer(0x12345678));
        source.Module().add_global_variable(table);
        source.Module().add_global_variable(guard);
        source.Set(1, source.Buffer(1, 0));
        auto* entry = context.construct<LLVMBC::BasicBlock>(context);
        auto* active = context.construct<LLVMBC::BasicBlock>(context);
        auto* merge = context.construct<LLVMBC::BasicBlock>(context);
        block = entry;
        auto* handle = source.Handle(1, 4);
        block->add_instruction(handle);
        auto* thread = call("dx.op.threadId.i32", 93, word, {integer(93), integer(0)});
        auto* local = emit<LLVMBC::AllocaInst>(LLVMBC::PointerType::get(word, 0), word, integer(1));
        (void)emit<LLVMBC::StoreInst>(local, thread);
        auto* condition = emit<LLVMBC::ICmpInst>(LLVMBC::Instruction::ICMP_NE, binary(thread, integer(1), LLVMBC::Instruction::And), integer(0));
        if (mode == 4) condition = emit<LLVMBC::ICmpInst>(LLVMBC::Instruction::ICMP_EQ, thread, thread);
        (void)emit<LLVMBC::BranchInst>(active, merge, condition);
        block = active;
        auto* loadedLocal = emit<LLVMBC::LoadInst>(word, local);
        auto* pattern = binary(binary(loadedLocal, integer(17), LLVMBC::Instruction::Mul), integer(5), LLVMBC::Instruction::Add);
        LLVMBC::Value* payload = pattern;
        if (mode == 2) payload = emit<LLVMBC::CastInst>(element, pattern, LLVMBC::Instruction::BitCast);
        if (mode == 3) payload = binary(emit<LLVMBC::CastInst>(element, pattern, LLVMBC::Instruction::ZExt), LLVMBC::ConstantInt::get(element, 0xDEADBEEF00000000), LLVMBC::Instruction::Or);
        LLVMBC::Value* index = constantIndex ? static_cast<LLVMBC::Value*>(integer(static_cast<std::uint32_t>(*constantIndex))) : binary(thread, integer(7), LLVMBC::Instruction::And);
        if (outside) index = binary(thread, integer(8), LLVMBC::Instruction::Add);
        auto* flatType = LLVMBC::PointerType::get(element, mode == 8 ? 2u : 0u);
        LLVMBC::Value* flat = expressions ? static_cast<LLVMBC::Value*>(context.construct<LLVMBC::ConstantExpr>(LLVMBC::Instruction::BitCast, flatType, LLVMBC::Vector<LLVMBC::Value*>{table})) : emit<LLVMBC::CastInst>(flatType, table, LLVMBC::Instruction::BitCast);
        auto* pointer = emit<LLVMBC::GetElementPtrInst>(flatType, LLVMBC::Vector<LLVMBC::Value*>{flat, index}, false);
        (void)emit<LLVMBC::StoreInst>(pointer, payload);
        (void)emit<LLVMBC::StoreInst>(guard, loadedLocal);
        (void)emit<LLVMBC::BranchInst>(merge);
        block = merge;
        index = constantIndex ? static_cast<LLVMBC::Value*>(integer(static_cast<std::uint32_t>(*constantIndex))) : binary(thread, integer(7), LLVMBC::Instruction::And);
        if (outside) index = binary(thread, integer(8), LLVMBC::Instruction::Add);
        flat = expressions ? static_cast<LLVMBC::Value*>(context.construct<LLVMBC::ConstantExpr>(LLVMBC::Instruction::BitCast, flatType, LLVMBC::Vector<LLVMBC::Value*>{table})) : emit<LLVMBC::CastInst>(flatType, table, LLVMBC::Instruction::BitCast);
        pointer = emit<LLVMBC::GetElementPtrInst>(flatType, LLVMBC::Vector<LLVMBC::Value*>{flat, index}, false);
        LLVMBC::Value* value = emit<LLVMBC::LoadInst>(element, pointer);
        LLVMBC::Value* high = integer(0);
        if (mode == 2) value = emit<LLVMBC::CastInst>(word, value, LLVMBC::Instruction::BitCast);
        if (mode == 3) {
            high = emit<LLVMBC::CastInst>(word, binary(value, LLVMBC::ConstantInt::get(element, 32), LLVMBC::Instruction::LShr), LLVMBC::Instruction::Trunc);
            value = emit<LLVMBC::CastInst>(word, value, LLVMBC::Instruction::Trunc);
        }
        auto* offset = binary(thread, integer(12), LLVMBC::Instruction::Mul);
        store(handle, offset, value);
        store(handle, binary(offset, integer(4), LLVMBC::Instruction::Add), high);
        store(handle, binary(offset, integer(8), LLVMBC::Instruction::Add), emit<LLVMBC::LoadInst>(word, guard));
        (void)emit<LLVMBC::ReturnInst>(nullptr);
        auto* functionType = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
        function = context.construct<LLVMBC::Function>(functionType, 1024, source.Module());
        function->set_basic_blocks({entry, active, merge});
    }
    ShaderRecompiler::IrProgram Lower() {
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
    LLVMBC::CallInst* call(const std::string& name, unsigned id, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        auto& context = source.Context();
        source.Module().add_value_name(id, LLVMBC::String(name.begin(), name.end()));
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* type = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* called = context.construct<LLVMBC::Function>(type, id, source.Module());
        return emit<LLVMBC::CallInst>(type, called, std::move(arguments));
    }
    void store(LLVMBC::Value* handle, LLVMBC::Value* offset, LLVMBC::Value* value) {
        auto* unused = LLVMBC::UndefValue::get(LLVMBC::Type::getInt32Ty(source.Context()));
        auto* mask = LLVMBC::ConstantInt::get(LLVMBC::Type::getInt8Ty(source.Context()), 1);
        (void)call("dx.op.rawBufferStore.i32", 140, LLVMBC::Type::getVoidTy(source.Context()), {integer(140), handle, offset, unused, value, unused, unused, unused, mask, integer(4)});
    }
    ResourceTests::Fixture source;
    PrivateGlobalBindings bindings;
    LLVMBC::BasicBlock* block;
    LLVMBC::Function* function;
};
}
