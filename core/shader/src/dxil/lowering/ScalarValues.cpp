#include "Scalar.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

unsigned ScalarLowering::width(const LLVMBC::Value& value) {
    auto* type = value.getType();
    if (!type || !type->isIntegerTy()) throw std::runtime_error("DXIL scalar lowering requires an integer value: kind=" + std::to_string(static_cast<unsigned>(value.get_value_kind())) + " type=" + std::to_string(type ? static_cast<unsigned>(type->getTypeID()) : 0));
    auto bits = type->getIntegerBitWidth();
    if (bits != 1 && bits != 8 && bits != 16 && bits != 32 && bits != 64) throw std::runtime_error("DXIL integer width is not implemented: " + std::to_string(bits));
    return bits;
}

IrType ScalarLowering::GetType(const LLVMBC::Value& value) {
    if (value.getType() && value.getType()->getTypeID() == LLVMBC::Type::TypeID::FloatTyID) return IrType::F32;
    auto bits = width(value);
    return bits == 1 ? IrType::Bool : bits == 64 ? IrType::U64 : IrType::U32;
}

IrValue& ScalarLowering::unsignedValue(IrValue& value) {
    if (value.Type() == IrType::Bool) return builder.Select(value, builder.Constant(1), builder.Constant(0));
    if (value.Type() != IrType::U32) throw std::logic_error("DXIL integer value has an incompatible IR type");
    return value;
}

IrValue& ScalarLowering::narrow(IrValue& value, unsigned bits) {
    if (bits == 64) {
        if (value.Type() != IrType::U64) throw std::logic_error("DXIL wide integer value has an incompatible IR type");
        return value;
    }
    auto& integer = value.Type() == IrType::U64 ? builder.CompositeExtract(value, 0) : unsignedValue(value);
    if (bits == 32) return integer;
    auto& masked = builder.BitwiseAnd(integer, builder.Constant((1u << bits) - 1));
    if (bits == 1) return builder.INotEqual(masked, builder.Constant(0));
    return masked;
}

IrValue& ScalarLowering::signedValue(IrValue& value, unsigned bits) {
    auto& integer = unsignedValue(value);
    if (bits == 32) return integer;
    auto& shift = builder.Constant(32 - bits);
    return builder.ShiftRightArithmetic(builder.ShiftLeftLogical(integer, shift), shift);
}

void ScalarLowering::Bind(const LLVMBC::Value& source, IrValue& value) {
    if (values.contains(&source)) throw std::logic_error("DXIL value is already bound");
    auto expected = GetType(source);
    if (value.IsPhi() || expected == IrType::F32) {
        if (value.Type() != expected) throw std::logic_error("DXIL scalar binding has an incompatible IR type");
        values.emplace(&source, &value);
        return;
    }
    values.emplace(&source, &narrow(value, width(source)));
}
