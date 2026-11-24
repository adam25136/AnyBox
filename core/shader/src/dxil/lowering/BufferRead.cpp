#include "Buffer.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::BufferLowering;

IrValue& BufferLowering::read(IrValue& address, IrValue& active, unsigned bits, bool coherent) {
    auto& low = builder.CompositeExtract(address, 0);
    auto& high = builder.CompositeExtract(address, 1);
    auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    auto flags = memory(bits, coherent);
    auto opcode = bits == 8 ? IrOpcode::LoadAddressU8 : bits == 16 ? IrOpcode::LoadAddressU16 : bits == 64 ? IrOpcode::LoadAddressU32x2 : IrOpcode::LoadAddressU32;
    auto& loaded = builder.Emit(opcode, IrOpcodeType(opcode), {&resource, &low, &high, &active}, flags);
    if (bits == 8) return builder.Emit(IrOpcode::ConvertU32U8, IrType::U32, {&loaded});
    if (bits == 16) return builder.Emit(IrOpcode::ConvertU32U16, IrType::U32, {&loaded});
    if (bits == 64) return builder.ConstructU64(builder.CompositeExtract(loaded, 0), builder.CompositeExtract(loaded, 1));
    return loaded;
}
