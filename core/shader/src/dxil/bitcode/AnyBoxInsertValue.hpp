#pragma once

#include <context.hpp>
#include <instruction.hpp>
#include <type.hpp>
#include <cstdint>
#include <limits>
#include <span>

namespace AnyBox::Shader::Dxil::Bitcode {
inline LLVMBC::Value* InsertValue(LLVMBC::LLVMContext& context, LLVMBC::Value& aggregate, LLVMBC::Value& inserted, std::span<const std::uint64_t> indices) {
    using namespace LLVMBC;
    if (indices.empty()) return nullptr;
    auto* type = aggregate.getType();
    auto kind = type->getTypeID();
    if (kind != Type::TypeID::StructTyID && kind != Type::TypeID::ArrayTyID) return nullptr;
    auto count = kind == Type::TypeID::StructTyID ? type->getStructNumElements() : type->getArrayNumElements();
    if (count > std::numeric_limits<unsigned>::max() || indices.front() >= count) return nullptr;
    Vector<Value*> constituents;
    constituents.reserve(static_cast<unsigned>(count));
    for (unsigned index = 0; index < count; ++index) {
        auto* element = kind == Type::TypeID::StructTyID ? type->getStructElementType(index) : type->getArrayElementType();
        Value* value;
        if (aggregate.get_value_kind() == ValueKind::Undef) value = UndefValue::get(element);
        else if (aggregate.get_value_kind() == ValueKind::CompositeConstruct) value = static_cast<CompositeConstructInst&>(aggregate).getOperand(index);
        else value = context.construct<ExtractValueInst>(element, &aggregate, Vector<unsigned>{index});
        if (index == indices.front()) {
            if (indices.size() == 1) {
                if (element != inserted.getType()) return nullptr;
                value = &inserted;
            } else {
                value = InsertValue(context, *value, inserted, indices.subspan(1));
                if (!value) return nullptr;
            }
        }
        constituents.push_back(value);
    }
    return context.construct<CompositeConstructInst>(type, std::move(constituents));
}
}
