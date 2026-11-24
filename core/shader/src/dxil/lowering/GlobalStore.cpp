#include "Scalar.hpp"
#include <cast.hpp>
#include <array>
#include <limits>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::globalStoreData(const LLVMBC::Value& value, unsigned words) {
    if (words == 1) {
        if (width(value) != 32) throw std::runtime_error("Xbox global store requires a 32-bit payload");
        return Get(value);
    }
    auto* type = value.getType();
    if (!type || type->getTypeID() != LLVMBC::Type::TypeID::StructTyID || type->getStructNumElements() != words) throw std::runtime_error("Xbox global store requires a structure of words");
    for (unsigned index = 0; index < words; ++index) {
        auto* member = type->getStructElementType(index);
        if (!member || !member->isIntegerTy() || member->getIntegerBitWidth() != 32) throw std::runtime_error("Xbox global store payload contains a non-word member");
    }
    auto* composite = LLVMBC::dyn_cast<LLVMBC::CompositeConstructInst>(&value);
    auto* constants = LLVMBC::dyn_cast<LLVMBC::ConstantDataArray>(&value);
    auto* aggregate = LLVMBC::dyn_cast<LLVMBC::ConstantAggregate>(&value);
    if (composite && composite->getNumOperands() != words) throw std::runtime_error("Xbox global store payload has an invalid component count");
    if (constants && constants->getNumElements() != words) throw std::runtime_error("Xbox global store constant payload has an invalid component count");
    if (aggregate && aggregate->getNumOperands() != words) throw std::runtime_error("Xbox global store aggregate payload has an invalid component count");
    bool zero = LLVMBC::isa<LLVMBC::UndefValue>(&value) || LLVMBC::isa<LLVMBC::ConstantAggregateZero>(&value);
    if (!composite && !constants && !aggregate && !zero) {
        auto& result = Get(value);
        if (!IsVectorType(result.Type()) || ComponentCount(result.Type()) != words) throw std::runtime_error("Xbox global store payload has an incompatible IR type");
        return result;
    }

    std::array<IrValue*, 4> components{};
    for (unsigned index = 0; index < words; ++index) {
        auto* element = composite ? composite->getOperand(index) : constants ? constants->getElementAsConstant(index) : aggregate ? aggregate->getOperand(index) : nullptr;
        if (element && width(*element) != 32) throw std::runtime_error("Xbox global store payload component is not a word");
        components[index] = element ? &Get(*element) : &builder.Constant(0);
    }
    constexpr std::array operations{IrOpcode::CompositeConstructU32x2, IrOpcode::CompositeConstructU32x3, IrOpcode::CompositeConstructU32x4};
    auto operation = operations.at(words - 2);
    if (words == 2) return builder.Emit(operation, IrOpcodeType(operation), {components[0], components[1]});
    if (words == 3) return builder.Emit(operation, IrOpcodeType(operation), {components[0], components[1], components[2]});
    return builder.Emit(operation, IrOpcodeType(operation), {components[0], components[1], components[2], components[3]});
}

IrValue& ScalarLowering::globalStore(const LLVMBC::CallInst& instruction, unsigned words) {
    if (words < 1 || words > 4 || instruction.getNumArgOperands() != 5 || !instruction.getType() || instruction.getType()->getTypeID() != LLVMBC::Type::TypeID::VoidTyID) throw std::runtime_error("Xbox global store has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || width(*opcode) != 32 || opcode->getUniqueInteger().getZExtValue() != 359 + words) throw std::runtime_error("Xbox global store has an invalid opcode");
    auto* input = instruction.getArgOperand(1);
    if (width(*input) != 64) throw std::runtime_error("Xbox global store requires a 64-bit guest address");
    for (unsigned index = 3; index < 5; ++index) {
        auto* control = instruction.getArgOperand(index);
        if (width(*control) != 32 || !LLVMBC::isa<LLVMBC::UndefValue>(control)) throw std::runtime_error("Xbox explicit global store cache controls are not implemented");
    }

    auto& data = globalStoreData(*instruction.getArgOperand(2), words);
    auto& address = Get(*input);
    auto& low = builder.CompositeExtract(address, 0);
    auto& high = builder.CompositeExtract(address, 1);
    auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    memory.dataDwords = memory.componentCount = words;
    auto& metadata = program.Resources().memoryInfo;
    if (metadata.size() >= std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL memory operation count exceeds the IR limit");
    MemoryFlags flags{static_cast<std::uint32_t>(metadata.size()), 0};
    metadata.push_back(memory);
    constexpr std::array operations{IrOpcode::StoreAddressU32, IrOpcode::StoreAddressU32x2, IrOpcode::StoreAddressU32x3, IrOpcode::StoreAddressU32x4};
    return builder.Emit(operations.at(words - 1), IrType::Void, {&resource, &low, &high, &data, &builder.ConstantBool(true)}, flags);
}
