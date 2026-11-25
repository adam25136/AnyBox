#pragma once

#include "HalfMemoryBase.hpp"
#include <optional>

namespace BufferTests {
inline constexpr std::array<std::uint32_t, 9> HalfMemoryWords{0, 0x8000, 1, 0x03FF, 0x0400, 0x7BFF, 0x7C00, 0x7D13, 0xFE47};

class HalfMemoryFixture : public HalfMemoryBase {
public:
    HalfMemoryFixture(bool floating, unsigned mode, std::optional<std::int32_t> fixed = {}, bool outside = false) : HalfMemoryBase(floating) {
        auto& context = source.Context();
        auto count = mode == 0 ? 3u : mode == 1 ? 65u : 9u;
        auto* array = LLVMBC::ArrayType::get(element, count);
        LLVMBC::Value* table;
        LLVMBC::Value* guard;
        if (mode == 0) {
            table = Emit<LLVMBC::AllocaInst>(LLVMBC::PointerType::get(array, 0), array, Integer(1));
            guard = Emit<LLVMBC::AllocaInst>(LLVMBC::PointerType::get(element, 0), element, Integer(1));
        } else {
            auto space = mode == 1 ? 3u : 0u;
            auto* variable = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(array, space), LLVMBC::GlobalVariable::InternalLinkage, mode == 3);
            LLVMBC::Vector<LLVMBC::Value*> values;
            for (auto bits : HalfMemoryWords) values.push_back(Value(bits));
            variable->set_initializer(mode == 1 ? static_cast<LLVMBC::Constant*>(LLVMBC::UndefValue::get(array)) : context.construct<LLVMBC::ConstantAggregate>(array, std::move(values)));
            table = variable;
            auto* marker = context.construct<LLVMBC::GlobalVariable>(LLVMBC::PointerType::get(element, space), LLVMBC::GlobalVariable::InternalLinkage, mode == 3);
            marker->set_initializer(mode == 1 ? LLVMBC::UndefValue::get(element) : static_cast<LLVMBC::Constant*>(Value(0x7D55)));
            guard = marker;
            source.Module().add_global_variable(variable);
            source.Module().add_global_variable(marker);
        }
        auto* value = Binary(Binary(global, Integer(17), LLVMBC::Instruction::Mul), Integer(0x7B00), LLVMBC::Instruction::Add);
        if (mode == 0) {
            for (unsigned index = 0; index < 3; ++index) Emit<LLVMBC::StoreInst>(Pointer(table, Integer(index)), Narrow(Binary(value, Integer(index), LLVMBC::Instruction::Add)));
            Emit<LLVMBC::StoreInst>(guard, Value(0x7D55));
        }
        if (mode == 1) {
            auto* active = context.construct<LLVMBC::BasicBlock>(context);
            auto* merge = context.construct<LLVMBC::BasicBlock>(context);
            blocks.push_back(active);
            blocks.push_back(merge);
            auto* first = Emit<LLVMBC::ICmpInst>(LLVMBC::Instruction::ICMP_EQ, local, Integer(0));
            Emit<LLVMBC::BranchInst>(active, merge, first);
            block = active;
            Emit<LLVMBC::StoreInst>(Pointer(table, Integer(0)), Value(0xD35A));
            Emit<LLVMBC::StoreInst>(guard, Value(0x7D55));
            Emit<LLVMBC::BranchInst>(merge);
            block = merge;
            Emit<LLVMBC::StoreInst>(Pointer(table, Binary(local, Integer(1), LLVMBC::Instruction::Add)), Narrow(value));
            Call("dx.op.barrier", LLVMBC::Type::getVoidTy(context), {Integer(80), Integer(9)});
        }
        LLVMBC::Value* index = mode == 1 ? Binary(Binary(local, Integer(1), LLVMBC::Instruction::Xor), Integer(1), LLVMBC::Instruction::Add) : Binary(global, Integer(count), LLVMBC::Instruction::URem);
        if (fixed) index = Integer(static_cast<std::uint32_t>(*fixed));
        if (outside) index = Binary(index, Integer(count), LLVMBC::Instruction::Add);
        auto* selected = Pointer(table, index);
        auto* initial = Emit<LLVMBC::LoadInst>(element, selected);
        Output(0, initial);
        if (mode == 2) {
            Emit<LLVMBC::StoreInst>(selected, Narrow(value));
            Output(1, Emit<LLVMBC::LoadInst>(element, selected));
        } else {
            auto* next = mode == 1 ? static_cast<LLVMBC::Value*>(Integer(0)) : Binary(Binary(index, Integer(1), LLVMBC::Instruction::Add), Integer(count), LLVMBC::Instruction::URem);
            Output(1, Emit<LLVMBC::LoadInst>(element, Pointer(table, next)));
        }
        Output(2, Emit<LLVMBC::LoadInst>(element, guard));
    }
};
}
