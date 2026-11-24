#include "Buffer.hpp"
#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::BufferLowering;

IrValue& BufferLowering::Atomic(const LLVMBC::CallInst& instruction, ScalarLowering& scalar) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 7 || !word(instruction)) throw std::runtime_error("DXIL buffer atomic has an invalid signature");
    if (constant(instruction, 0, 32) != 78) throw std::runtime_error("DXIL buffer atomic opcode is invalid");
    constexpr std::array operations{IrOpcode::AddressAtomicIAdd32, IrOpcode::AddressAtomicAnd32, IrOpcode::AddressAtomicOr32, IrOpcode::AddressAtomicXor32, IrOpcode::AddressAtomicSMin32, IrOpcode::AddressAtomicSMax32, IrOpcode::AddressAtomicUMin32, IrOpcode::AddressAtomicUMax32, IrOpcode::AddressAtomicSwap32};
    auto mode = constant(instruction, 2, 32);
    if (mode >= operations.size()) throw std::runtime_error("DXIL buffer atomic mode is not implemented");
    auto* unused = instruction.getArgOperand(5);
    if (!word(*unused) || !LLVMBC::isa<LLVMBC::UndefValue>(unused)) throw std::runtime_error("DXIL buffer atomic has an unsupported third coordinate");
    if (!word(*instruction.getArgOperand(6))) throw std::runtime_error("DXIL buffer atomic requires a 32-bit integer value");

    auto access = resolve(instruction, scalar, true, 3);
    auto selected = component(access, 32, 0);
    auto& low = builder.CompositeExtract(selected.address, 0);
    auto& high = builder.CompositeExtract(selected.address, 1);
    auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    auto& value = scalar.Get(*instruction.getArgOperand(6));
    return builder.Emit(operations[mode], IrType::U32, {&resource, &low, &high, &value, &selected.active}, memory(32, access.binding.globallyCoherent));
}
