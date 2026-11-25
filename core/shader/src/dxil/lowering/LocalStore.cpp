#include "Local.hpp"
#include "Scalar.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::LocalLowering;

void LocalLowering::store(const LLVMBC::StoreInst& instruction, ScalarLowering& scalar) {
    auto* source = instruction.getOperand(0);
    auto pointer = resolve(*instruction.getOperand(1), scalar);
    if (pointer.allocation.constants) throw std::runtime_error("DXIL cannot write a constant global variable");
    auto location = access(pointer, *source->getType());
    auto& input = scalar.Get(*source);
    bool narrow = size(*source->getType()) == 2;
    if (narrow) {
        auto& bits = builder.Emit(input.Type() == IrType::F16 ? IrOpcode::BitCastU16F16 : IrOpcode::ConvertU16U32, IrType::U16, {&input});
        if (pointer.allocation.shared) (void)builder.Emit(IrOpcode::WriteSharedU16, IrType::Void, {&location.offset, &bits, &location.active}, flags(true, 1, 16));
        else {
            auto& resource = builder.Emit(IrOpcode::GetScratchResource, IrType::AddressResource, {});
            (void)builder.Emit(IrOpcode::StoreAddressU16, IrType::Void, {&resource, &location.offset, &builder.Constant(0), &bits, &location.active}, flags(false, 1, 16));
        }
        return;
    }
    bool wide = input.Type() == IrType::U64;
    auto& bits = input.Type() == IrType::F32 ? builder.BitCastU32(input) : input;
    if (pointer.allocation.shared) {
        if (wide) (void)builder.Emit(IrOpcode::WriteSharedU32x2, IrType::Void, {&location.offset, &builder.CompositeExtract(bits, 0), &builder.CompositeExtract(bits, 1), &location.active}, flags(true, 2));
        else (void)builder.Emit(IrOpcode::WriteSharedU32, IrType::Void, {&location.offset, &bits, &location.active}, flags(true));
        return;
    }
    auto& resource = builder.Emit(IrOpcode::GetScratchResource, IrType::AddressResource, {});
    auto& low = wide ? builder.CompositeExtract(bits, 0) : bits;
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &location.offset, &builder.Constant(0), &low, &location.active}, flags(false));
    if (wide) {
        auto& highOffset = builder.IAdd(location.offset, builder.Constant(4));
        (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &highOffset, &builder.Constant(0), &builder.CompositeExtract(bits, 1), &location.active}, flags(false));
    }
}
