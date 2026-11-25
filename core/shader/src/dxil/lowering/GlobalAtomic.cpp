#include "Scalar.hpp"
#include <cast.hpp>
#include <limits>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::globalFloatAtomic(const LLVMBC::CallInst& instruction, bool maximum) {
    if (instruction.getNumArgOperands() != 3 || GetType(instruction) != IrType::F32) throw std::runtime_error("Xbox floating global atomic has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || width(*instruction.getArgOperand(1)) != 64 || GetType(*instruction.getArgOperand(2)) != IrType::F32) throw std::runtime_error("Xbox floating global atomic has invalid operand types");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != (maximum ? 369u : 368u)) throw std::runtime_error("Xbox floating global atomic opcode does not match its declaration");

    auto& value = builder.BitCastU32(Get(*instruction.getArgOperand(2)));
    auto operation = maximum ? IrOpcode::AddressAtomicFMax32 : IrOpcode::AddressAtomicFMin32;
    return builder.BitCastF32(globalAtomic(operation, Get(*instruction.getArgOperand(1)), value));
}

IrValue& ScalarLowering::globalAtomic(IrOpcode operation, IrValue& address, IrValue& value) {
    if (address.Type() != IrType::U64 || value.Type() != IrType::U32) throw std::logic_error("Xbox global atomic IR operand types are invalid");
    auto& low = builder.CompositeExtract(address, 0);
    auto& high = builder.CompositeExtract(address, 1);
    auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    auto& metadata = program.Resources().memoryInfo;
    if (metadata.size() >= std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL memory operation count exceeds the IR limit");
    MemoryFlags flags{static_cast<std::uint32_t>(metadata.size()), 0};
    metadata.push_back(memory);
    return builder.Emit(operation, IrType::U32, {&resource, &low, &high, &value, &builder.ConstantBool(true)}, flags);
}
