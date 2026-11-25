#pragma once

#include "AtomicKernel.hpp"
#include "lowering/Local.hpp"
#include "../../src/backend/AnyBoxBarrier.hpp"

namespace BufferTests {
inline constexpr std::array<std::uint64_t, 8> WideConstants{0, UINT64_MAX, 0x8000000000000000, 0x7FFFFFFFFFFFFFFF, 0x0123456789ABCDEF, 0xFEDCBA9876543210, 0xA5A5A5A55A5A5A5A, 0x00000001FFFFFFFF};

inline ShaderRecompiler::IrProgram WideStorageKernel(unsigned readMode = 0) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    AnyBox::Shader::Dxil::LocalLowering locals(fixture.program, builder);
    fixture.scalar.SetLocals(locals);
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* wide = LLVMBC::Type::getInt64Ty(context);
    auto integer = [&](std::uint64_t value) { return LLVMBC::ConstantInt::get(word, value); };
    auto* array = LLVMBC::ArrayType::get(wide, 64);
    auto* shared = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(array, 3), LLVMBC::GlobalVariable::InternalLinkage, false);
    shared->set_initializer(LLVMBC::UndefValue::get(array));
    auto* scratch = context.construct<LLVMBC::AllocaInst>(LLVMBC::PointerType::get(array, 0), array, integer(1));
    auto* block = context.construct<LLVMBC::BasicBlock>(context);
    block->add_instruction(scratch);
    auto* signature = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
    auto* function = context.construct<LLVMBC::Function>(signature, 0, fixture.source.Module());
    function->set_basic_blocks({block});
    locals.Prepare(*function);

    auto& global = fixture.GlobalIndex();
    auto& local = builder.BitwiseAnd(global, builder.Constant(63));
    auto& selected = builder.BitwiseXor(local, builder.Constant(32));
    auto* localArgument = context.construct<LLVMBC::Argument>(word, 0, false);
    auto* selectedArgument = context.construct<LLVMBC::Argument>(word, 1, false);
    fixture.scalar.Bind(*localArgument, local);
    fixture.scalar.Bind(*selectedArgument, readMode == 4 ? builder.IAdd(selected, builder.Constant(64)) : selected);
    auto pointer = [&](LLVMBC::Value* allocation, unsigned space, LLVMBC::Value* index) {
        return context.construct<LLVMBC::GetElementPtrInst>(LLVMBC::PointerType::get(wide, space), LLVMBC::Vector<LLVMBC::Value*>{allocation, integer(0), index}, true);
    };
    auto* sharedWrite = pointer(shared, 3, localArgument);
    auto* privatePointer = pointer(scratch, 0, localArgument);
    auto& low = builder.IAdd(builder.IMul(global, builder.Constant(17)), builder.Constant(5));
    auto& high = builder.BitwiseXor(global, builder.Constant(0xA5F03C69));
    auto& payload = builder.ConstructU64(low, high);
    auto* value = context.construct<LLVMBC::Argument>(wide, 2, false);
    fixture.scalar.Bind(*value, payload);
    auto* sharedStore = context.construct<LLVMBC::StoreInst>(sharedWrite, value);
    auto* privateStore = context.construct<LLVMBC::StoreInst>(privatePointer, value);
    (void)locals.Emit(*sharedStore, fixture.scalar);
    (void)locals.Emit(*privateStore, fixture.scalar);
    (void)builder.Emit(IrOpcode::Barrier, IrType::Void, {}, AnyBox::Shader::Backend::DxilBarrierTag | 9);

    LLVMBC::Value* readIndex = selectedArgument;
    if (readMode >= 1 && readMode <= 3) readIndex = integer(readMode == 1 ? 63 : readMode == 2 ? 64 : UINT32_MAX);
    LLVMBC::Value* sharedRead = pointer(shared, 3, readIndex);
    if (readMode == 5) {
        auto* last = pointer(shared, 3, integer(63));
        auto* cast = context.construct<LLVMBC::CastInst>(LLVMBC::PointerType::get(word, 3), last, LLVMBC::Instruction::BitCast);
        auto* tail = context.construct<LLVMBC::GetElementPtrInst>(LLVMBC::PointerType::get(word, 3), LLVMBC::Vector<LLVMBC::Value*>{cast, integer(1)}, true);
        sharedRead = context.construct<LLVMBC::CastInst>(LLVMBC::PointerType::get(wide, 3), tail, LLVMBC::Instruction::BitCast);
    }
    auto* tableType = LLVMBC::ArrayType::get(wide, WideConstants.size());
    auto* table = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(tableType, 0), LLVMBC::GlobalVariable::InternalLinkage, true);
    LLVMBC::Vector<LLVMBC::Value*> constants;
    for (auto item : WideConstants) constants.push_back(LLVMBC::ConstantInt::get(wide, item));
    LLVMBC::Constant* initializer = context.construct<LLVMBC::ConstantDataArray>(tableType, std::move(constants));
    if (readMode == 6) initializer = context.construct<LLVMBC::ConstantAggregateZero>(tableType);
    if (readMode == 7) initializer = LLVMBC::UndefValue::get(tableType);
    table->set_initializer(initializer);
    auto* constantIndex = context.construct<LLVMBC::Argument>(word, 3, false);
    fixture.scalar.Bind(*constantIndex, builder.BitwiseAnd(local, builder.Constant(7)));
    auto* constantPointer = pointer(table, 0, readMode == 1 ? static_cast<LLVMBC::Value*>(integer(7)) : constantIndex);
    std::array<LLVMBC::Value*, 3> pointers{sharedRead, privatePointer, constantPointer};
    auto& offset = builder.IMul(global, builder.Constant(24));
    for (unsigned item = 0; item < pointers.size(); ++item) {
        auto* load = context.construct<LLVMBC::LoadInst>(wide, pointers[item]);
        auto& loaded = locals.Load(*load, fixture.scalar);
        for (unsigned component = 0; component < 2; ++component) {
            auto& wordOffset = builder.IAdd(offset, builder.Constant(item * 8 + component * 4));
            fixture.Memory(IrOpcode::StoreAddressU32, wordOffset, 2, &builder.CompositeExtract(loaded, component));
        }
    }
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}
