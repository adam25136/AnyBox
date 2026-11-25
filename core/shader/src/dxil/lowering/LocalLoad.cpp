#include "Local.hpp"
#include "Scalar.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::LocalLowering;

IrValue& LocalLowering::Load(const LLVMBC::LoadInst& instruction, ScalarLowering& scalar) {
    auto pointer = resolve(*instruction.getPointerOperand(), scalar);
    auto location = access(pointer, *instruction.getType());
    bool half = instruction.getType()->getTypeID() == LLVMBC::Type::TypeID::HalfTyID;
    bool narrow = size(*instruction.getType()) == 2;
    bool wide = instruction.getType()->isIntegerTy() && instruction.getType()->getIntegerBitWidth() == 64;
    if (pointer.allocation.constants) {
        auto& value = constantWord(pointer, location);
        if (wide) return builder.ConstructU64(value, constantWord(pointer, location, 1));
        if (narrow) {
            auto& shift = builder.ShiftLeftLogical(builder.BitwiseAnd(builder.CompositeExtract(*pointer.offset, 0), builder.Constant(2)), builder.Constant(3));
            auto& bits = builder.BitwiseAnd(builder.ShiftRightLogical(value, shift), builder.Constant(65535));
            return half ? builder.BitCastF16(bits) : bits;
        }
        return instruction.getType()->getTypeID() == LLVMBC::Type::TypeID::FloatTyID ? builder.BitCastF32(value) : value;
    }
    auto type = wide ? IrType::Vec2U32 : narrow ? IrType::U16 : IrType::U32;
    IrValue* value;
    if (pointer.allocation.shared) {
        value = &builder.Emit(wide ? IrOpcode::LoadSharedU32x2 : narrow ? IrOpcode::LoadSharedU16 : IrOpcode::LoadSharedU32, type, {&location.offset, &location.active}, flags(true, wide ? 2 : 1, narrow ? 16 : 32));
    } else {
        auto& resource = builder.Emit(IrOpcode::GetScratchResource, IrType::AddressResource, {});
        value = &builder.Emit(narrow ? IrOpcode::LoadAddressU16 : IrOpcode::LoadAddressU32, narrow ? IrType::U16 : IrType::U32, {&resource, &location.offset, &builder.Constant(0), &location.active}, flags(false, 1, narrow ? 16 : 32));
        if (wide) {
            auto& highOffset = builder.IAdd(location.offset, builder.Constant(4));
            auto& high = builder.Emit(IrOpcode::LoadAddressU32, IrType::U32, {&resource, &highOffset, &builder.Constant(0), &location.active}, flags(false));
            return builder.ConstructU64(*value, high);
        }
    }
    if (wide) return builder.ConstructU64(builder.CompositeExtract(*value, 0), builder.CompositeExtract(*value, 1));
    if (narrow) return builder.Emit(half ? IrOpcode::BitCastF16U16 : IrOpcode::ConvertU32U16, half ? IrType::F16 : IrType::U32, {value});
    return instruction.getType()->getTypeID() == LLVMBC::Type::TypeID::FloatTyID ? builder.BitCastF32(*value) : *value;
}

