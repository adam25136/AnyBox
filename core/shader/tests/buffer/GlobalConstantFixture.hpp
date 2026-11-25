#pragma once

#include "AtomicKernel.hpp"
#include "GlobalWords.hpp"
#include "lowering/Local.hpp"
#include <optional>

namespace BufferTests {
inline ShaderRecompiler::IrProgram MakeConstantKernel(unsigned mode, std::optional<std::int32_t> constantIndex = {}, bool outside = false, bool write = false) {
    using namespace ShaderRecompiler;
    AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& globalIndex = fixture.GlobalIndex();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* element = mode == 2 ? LLVMBC::Type::getFloatTy(context) : word;
    auto* row = LLVMBC::ArrayType::get(element, 4);
    auto* table = LLVMBC::ArrayType::get(row, 2);
    LLVMBC::Vector<LLVMBC::Value*> rows;
    for (unsigned group = 0; group < 2; ++group) {
        LLVMBC::Vector<LLVMBC::Value*> values;
        for (unsigned part = 0; part < 4; ++part) {
            auto value = mode == 0 ? 0x3F800000U : ConstantWords[group * 4 + part];
            values.push_back(mode == 2 ? static_cast<LLVMBC::Value*>(LLVMBC::ConstantFP::get(element, value)) : LLVMBC::ConstantInt::get(element, value));
        }
        rows.push_back(context.construct<LLVMBC::ConstantDataArray>(row, std::move(values)));
    }
    LLVMBC::Constant* initializer = context.construct<LLVMBC::ConstantAggregate>(table, std::move(rows));
    if (mode == 3) initializer = context.construct<LLVMBC::ConstantAggregateZero>(table);
    if (mode == 4) initializer = LLVMBC::UndefValue::get(table);
    auto* variable = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(table, 0), LLVMBC::GlobalVariable::InternalLinkage, true);
    if (mode != 5) variable->set_initializer(initializer);
    fixture.source.Module().add_global_variable(variable);
    AnyBox::Shader::Dxil::LocalLowering locals(fixture.program, builder);
    fixture.scalar.SetLocals(locals);
    auto integer = [&](std::uint32_t value) { return LLVMBC::ConstantInt::get(word, value); };
    auto& selected = outside ? builder.IAdd(globalIndex, builder.Constant(8)) : builder.BitwiseAnd(globalIndex, builder.Constant(7));
    auto* input = context.construct<LLVMBC::Argument>(word, 0, false);
    fixture.scalar.Bind(*input, selected);
    LLVMBC::Value* index = constantIndex ? static_cast<LLVMBC::Value*>(integer(static_cast<std::uint32_t>(*constantIndex))) : input;
    auto* flat = context.construct<LLVMBC::CastInst>(LLVMBC::PointerType::get(element, 0), variable, LLVMBC::Instruction::BitCast);
    auto* pointer = context.construct<LLVMBC::GetElementPtrInst>(LLVMBC::PointerType::get(element, 0), LLVMBC::Vector<LLVMBC::Value*>{flat, index}, false);
    if (write) {
        auto* value = mode == 2 ? static_cast<LLVMBC::Value*>(LLVMBC::ConstantFP::get(element, 0)) : integer(0);
        auto* stored = context.construct<LLVMBC::StoreInst>(pointer, value);
        fixture.scalar.Emit(*stored);
    }
    auto* loaded = context.construct<LLVMBC::LoadInst>(element, pointer);
    auto& value = fixture.scalar.Get(*loaded);
    auto& bits = mode == 2 ? builder.BitCastU32(value) : value;
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(globalIndex, builder.Constant(2)), 2, &bits);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}
}
