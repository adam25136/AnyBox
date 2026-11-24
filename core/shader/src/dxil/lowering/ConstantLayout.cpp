#include "Local.hpp"
#include <cast.hpp>

using AnyBox::Shader::Dxil::LocalLowering;

void LocalLowering::appendConstants(const LLVMBC::Value& value, const LLVMBC::Type& type, std::vector<std::uint32_t>& words) {
    if (value.getType() != &type) throw std::runtime_error("DXIL global constant initializer type does not match its allocation");
    if (LLVMBC::isa<LLVMBC::UndefValue>(&value) || LLVMBC::isa<LLVMBC::ConstantAggregateZero>(&value)) {
        words.resize(words.size() + size(type) / 4, 0);
        return;
    }
    if (type.isIntegerTy() && type.getIntegerBitWidth() == 32) {
        auto* integer = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(&value);
        if (!integer) throw std::runtime_error("DXIL global constant word initializer is not an integer constant");
        words.push_back(static_cast<std::uint32_t>(integer->getUniqueInteger().getZExtValue()));
        return;
    }
    if (type.getTypeID() == LLVMBC::Type::TypeID::FloatTyID) {
        auto* floating = LLVMBC::dyn_cast<LLVMBC::ConstantFP>(&value);
        if (!floating) throw std::runtime_error("DXIL global constant float initializer is not a floating constant");
        words.push_back(static_cast<std::uint32_t>(floating->getValueAPF().bitcastToAPInt().getZExtValue()));
        return;
    }
    if (type.getTypeID() != LLVMBC::Type::TypeID::ArrayTyID) throw std::runtime_error("DXIL global constant aggregate type is not implemented");
    auto count = type.getArrayNumElements();
    auto* array = LLVMBC::dyn_cast<LLVMBC::ConstantDataArray>(&value);
    auto* aggregate = LLVMBC::dyn_cast<LLVMBC::ConstantAggregate>(&value);
    if ((!array && !aggregate) || (array && array->getNumElements() != count) || (aggregate && aggregate->getNumOperands() != count)) throw std::runtime_error("DXIL global constant array initializer has an invalid element count");
    for (unsigned index = 0; index < count; ++index) {
        auto* element = array ? array->getElementAsConstant(index) : aggregate->getOperand(index);
        appendConstants(*element, *type.getArrayElementType(), words);
    }
}

LocalLowering::Allocation LocalLowering::constantAllocation(const LLVMBC::GlobalVariable& variable) {
    auto* pointer = variable.getType();
    if (!pointer || pointer->getTypeID() != LLVMBC::Type::TypeID::PointerTyID || pointer->getPointerAddressSpace() != 0 || !variable.isConstant()) throw std::runtime_error("DXIL global constant requires an address-space-zero constant variable");
    if (!variable.hasInitializer()) throw std::runtime_error("DXIL global constant has no initializer");
    auto bytes = size(*pointer->getPointerElementType());
    auto words = std::make_shared<std::vector<std::uint32_t>>();
    words->reserve(bytes / 4);
    appendConstants(*variable.getInitializer(), *pointer->getPointerElementType(), *words);
    if (words->size() != bytes / 4) throw std::logic_error("DXIL global constant flattened size does not match its allocation");
    Allocation allocation{0, bytes, false, std::move(words)};
    allocations.emplace(&variable, allocation);
    return allocation;
}
