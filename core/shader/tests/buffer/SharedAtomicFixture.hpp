#pragma once

#include "AtomicKernel.hpp"
#include "lowering/Local.hpp"
#include "bitcode/AtomicAttributes.hpp"
#include "../../src/backend/AnyBoxBarrier.hpp"

namespace BufferTests {
void CheckSharedAtomic(AnyBox::Vulkan::Context& context);
using SharedOperation = LLVMBC::AtomicRMWInst::BinOp;
inline constexpr std::array<std::uint32_t, 16> SharedAtomicValues{0, 1, 2, 3, UINT32_MAX, UINT32_MAX - 1, 0x80000000, 0x7FFFFFFF, 0xAAAAAAAA, 0x55555555, 0xFFFF0000, 0x0000FFFF, 31, 32, 0x80000001, 0x7FFFFFFE};

struct SharedAtomicFixture {
    AtomicKernel fixture;
    AnyBox::Shader::Dxil::LocalLowering locals{fixture.program, fixture.builder};
    LLVMBC::Type* word = LLVMBC::Type::getInt32Ty(fixture.source.Context());
    LLVMBC::GlobalVariable* allocation;

    SharedAtomicFixture(unsigned space = 3, bool constant = false) {
        fixture.scalar.SetLocals(locals);
        auto& context = fixture.source.Context();
        auto* array = LLVMBC::ArrayType::get(word, 256);
        allocation = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(array, space), LLVMBC::GlobalVariable::InternalLinkage, constant);
        LLVMBC::Constant* initializer = LLVMBC::UndefValue::get(array);
        if (constant) initializer = context.construct<LLVMBC::ConstantAggregateZero>(array);
        allocation->set_initializer(initializer);
    }
    LLVMBC::Value* Integer(std::uint32_t value) { return LLVMBC::ConstantInt::get(word, value); }
    LLVMBC::Value* Bind(ShaderRecompiler::IrValue& value, unsigned id) {
        auto* argument = fixture.source.Context().construct<LLVMBC::Argument>(word, id, false);
        fixture.scalar.Bind(*argument, value);
        return argument;
    }
    LLVMBC::Value* Pointer(LLVMBC::Value* index) {
        auto space = allocation->getType()->getPointerAddressSpace();
        return fixture.source.Context().construct<LLVMBC::GetElementPtrInst>(LLVMBC::PointerType::get(word, space), LLVMBC::Vector<LLVMBC::Value*>{allocation, Integer(0), index}, true);
    }
    LLVMBC::AtomicRMWInst* Instruction(LLVMBC::Value* pointer, LLVMBC::Value* value, SharedOperation operation) {
        return fixture.source.Context().construct<LLVMBC::AtomicRMWInst>(value->getType(), pointer, value, operation);
    }
    void Barrier() {
        (void)fixture.builder.Emit(ShaderRecompiler::IrOpcode::Barrier, ShaderRecompiler::IrType::Void, {}, AnyBox::Shader::Backend::DxilBarrierTag | 9);
    }
};

inline ShaderRecompiler::IrProgram SharedAtomicKernel(SharedOperation operation, unsigned ordering, bool contention = false) {
    using namespace ShaderRecompiler;
    SharedAtomicFixture source;
    auto& fixture = source.fixture;
    auto& builder = fixture.builder;
    auto& global = fixture.GlobalIndex();
    auto& local = builder.BitwiseAnd(global, builder.Constant(255));
    auto& row = builder.BitwiseAnd(builder.ShiftRightLogical(global, builder.Constant(4)), builder.Constant(15));
    auto& column = builder.BitwiseAnd(global, builder.Constant(15));
    auto& initial = fixture.Memory(IrOpcode::LoadAddressU32, builder.IMul(row, builder.Constant(4)), 1);
    auto& operand = fixture.Memory(IrOpcode::LoadAddressU32, builder.IMul(column, builder.Constant(4)), 1);
    auto* pointer = source.Pointer(contention ? source.Integer(0) : source.Bind(local, 0));
    auto* value = contention ? source.Integer(1) : source.Bind(operand, 1);
    auto& initialize = builder.IEqual(local, builder.Constant(0));
    auto* store = fixture.source.Context().construct<LLVMBC::StoreInst>(pointer, contention ? source.Integer(0) : source.Bind(initial, 2));
    (void)source.locals.Emit(*store, fixture.scalar);
    if (contention) {
        const auto& instructions = fixture.program.EntryBlock().Instructions();
        for (auto* instruction : instructions) {
            if (instruction->Opcode() == IrOpcode::WriteSharedU32) instruction->ReplaceArgument(instruction->ArgumentCount() - 1, &initialize);
        }
    }
    source.Barrier();
    auto* atomic = source.Instruction(pointer, value, operation);
    std::array<std::uint64_t, 3> attributes{0, ordering, 1};
    if (!AnyBox::Shader::Dxil::Bitcode::StoreAtomicAttributes(*atomic, attributes)) throw std::runtime_error("Invalid shared atomic fixture attributes");
    auto& previous = fixture.scalar.Get(*atomic);
    source.Barrier();
    auto* load = fixture.source.Context().construct<LLVMBC::LoadInst>(source.word, pointer);
    auto& current = source.locals.Load(*load, fixture.scalar);
    auto& resultIndex = contention ? builder.IAdd(builder.BitwiseAnd(global, builder.Constant(~255u)), previous) : global;
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IMul(resultIndex, builder.Constant(8)), 2, &previous);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(builder.IMul(global, builder.Constant(8)), builder.Constant(4)), 2, &current);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}
