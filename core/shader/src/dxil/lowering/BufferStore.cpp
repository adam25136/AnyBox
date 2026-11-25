#include "Buffer.hpp"
#include "Scalar.hpp"
#include <bit>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::BufferLowering;

void BufferLowering::store(const LLVMBC::CallInst& instruction, unsigned bits, ScalarLowering& scalar, bool floating) {
    if (instruction.getNumArgOperands() != 10 || !instruction.getType() || instruction.getType()->getTypeID() != LLVMBC::Type::TypeID::VoidTyID) throw std::runtime_error("DXIL raw buffer store has an invalid signature");
    if (constant(instruction, 0, 32) != 140) throw std::runtime_error("DXIL raw buffer store opcode is invalid");
    auto mask = constant(instruction, 8, 8);
    auto alignment = constant(instruction, 9, 32);
    if (mask == 0 || mask > 15 || !std::has_single_bit(alignment)) throw std::runtime_error("DXIL raw buffer store mask or alignment is invalid");
    for (unsigned index = 4; index < 8; ++index) {
        auto* type = instruction.getArgOperand(index)->getType();
        auto floatingType = bits == 16 ? LLVMBC::Type::TypeID::HalfTyID : LLVMBC::Type::TypeID::FloatTyID;
        bool valid = type && (floating ? type->getTypeID() == floatingType : type->isIntegerTy() && type->getIntegerBitWidth() == bits);
        if (!valid) throw std::runtime_error("DXIL raw buffer store value type is invalid");
    }
    auto access = resolve(instruction, scalar, true);
    for (unsigned index = 0; index < 4; ++index) {
        if ((mask & (1u << index)) == 0) continue;
        auto part = component(access, bits, index);
        auto& value = scalar.Get(*instruction.getArgOperand(4 + index));
        auto& data = floating ? (bits == 16 ? builder.BitCastU32FromF16(value) : builder.BitCastU32(value)) : value;
        write(part.address, part.active, data, bits, access.binding.globallyCoherent);
    }
}

void BufferLowering::write(IrValue& address, IrValue& active, IrValue& value, unsigned bits, bool coherent) {
    auto& low = builder.CompositeExtract(address, 0);
    auto& high = builder.CompositeExtract(address, 1);
    auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    auto flags = memory(bits, coherent);
    IrValue* data = &value;
    if (bits == 16) data = &builder.Emit(IrOpcode::ConvertU16U32, IrType::U16, {&value});
    if (bits == 64) data = &builder.Emit(IrOpcode::CompositeConstructU32x2, IrType::Vec2U32, {&builder.CompositeExtract(value, 0), &builder.CompositeExtract(value, 1)});
    auto opcode = bits == 16 ? IrOpcode::StoreAddressU16 : bits == 64 ? IrOpcode::StoreAddressU32x2 : IrOpcode::StoreAddressU32;
    (void)builder.Emit(opcode, IrType::Void, {&resource, &low, &high, data, &active}, flags);
}
