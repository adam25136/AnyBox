#include "Local.hpp"
#include <cast.hpp>

using AnyBox::Shader::Dxil::LocalLowering;

void LocalLowering::appendConstants(const LLVMBC::Value& value, const LLVMBC::Type& type, std::vector<std::uint32_t>& words) {
    if (!words.empty()) throw std::logic_error("DXIL initializer output is already populated");
    auto bytes = size(type);
    words.resize(bytes / 4 + static_cast<unsigned>(bytes % 4 != 0));
    std::uint32_t byteOffset = 0;
    appendInitializer(value, type, words, byteOffset);
    if (byteOffset != bytes) throw std::logic_error("DXIL initializer byte size differs from its allocation");
}

void LocalLowering::appendInitializer(const LLVMBC::Value& value, const LLVMBC::Type& type, std::vector<std::uint32_t>& words, std::uint32_t& byteOffset) {
    if (value.getType() != &type) throw std::runtime_error("DXIL global constant initializer type does not match its allocation");
    if (LLVMBC::isa<LLVMBC::UndefValue>(&value) || LLVMBC::isa<LLVMBC::ConstantAggregateZero>(&value)) {
        byteOffset += size(type);
        return;
    }
    auto append = [&](std::uint64_t data) {
        auto bytes = size(type);
        for (unsigned index = 0; index < bytes; ++index) {
            auto offset = byteOffset + index;
            words.at(offset / 4) |= static_cast<std::uint32_t>((data >> (index * 8)) & 255) << ((offset % 4) * 8);
        }
        byteOffset += bytes;
    };
    if (type.isIntegerTy() && (type.getIntegerBitWidth() == 16 || type.getIntegerBitWidth() == 32 || type.getIntegerBitWidth() == 64)) {
        auto* integer = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(&value);
        if (!integer) throw std::runtime_error("DXIL global constant word initializer is not an integer constant");
        append(integer->getUniqueInteger().getZExtValue());
        return;
    }
    if (type.getTypeID() == LLVMBC::Type::TypeID::FloatTyID || type.getTypeID() == LLVMBC::Type::TypeID::HalfTyID) {
        auto* floating = LLVMBC::dyn_cast<LLVMBC::ConstantFP>(&value);
        if (!floating) throw std::runtime_error("DXIL global constant float initializer is not a floating constant");
        append(floating->getValueAPF().bitcastToAPInt().getZExtValue());
        return;
    }
    if (type.getTypeID() != LLVMBC::Type::TypeID::ArrayTyID) throw std::runtime_error("DXIL global constant aggregate type is not implemented");
    auto count = type.getArrayNumElements();
    auto* array = LLVMBC::dyn_cast<LLVMBC::ConstantDataArray>(&value);
    auto* aggregate = LLVMBC::dyn_cast<LLVMBC::ConstantAggregate>(&value);
    if ((!array && !aggregate) || (array && array->getNumElements() != count) || (aggregate && aggregate->getNumOperands() != count)) throw std::runtime_error("DXIL global constant array initializer has an invalid element count");
    for (unsigned index = 0; index < count; ++index) {
        auto* element = array ? array->getElementAsConstant(index) : aggregate->getOperand(index);
        appendInitializer(*element, *type.getArrayElementType(), words, byteOffset);
    }
}

LocalLowering::Allocation LocalLowering::constantAllocation(const LLVMBC::GlobalVariable& variable) {
    auto* pointer = variable.getType();
    if (!pointer || pointer->getTypeID() != LLVMBC::Type::TypeID::PointerTyID || pointer->getPointerAddressSpace() != 0 || !variable.isConstant()) throw std::runtime_error("DXIL global constant requires an address-space-zero constant variable");
    if (!variable.hasInitializer()) throw std::runtime_error("DXIL global constant has no initializer");
    auto bytes = size(*pointer->getPointerElementType());
    auto words = std::make_shared<std::vector<std::uint32_t>>();
    auto wordCount = bytes / 4 + static_cast<unsigned>(bytes % 4 != 0);
    words->reserve(wordCount);
    appendConstants(*variable.getInitializer(), *pointer->getPointerElementType(), *words);
    if (words->size() != wordCount) throw std::logic_error("DXIL global constant flattened size does not match its allocation");
    Allocation allocation{0, bytes, false, std::move(words)};
    allocations.emplace(&variable, allocation);
    return allocation;
}
