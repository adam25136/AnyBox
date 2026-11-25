#include "HalfScalar.hpp"
#include "Scalar.hpp"

using namespace ShaderRecompiler;
using namespace AnyBox::Shader::Dxil;

bool AnyBox::Shader::Dxil::IsHalf(const LLVMBC::Value& value) {
    return value.getType() && value.getType()->getTypeID() == LLVMBC::Type::TypeID::HalfTyID;
}

IrValue& AnyBox::Shader::Dxil::LowerHalfCast(const LLVMBC::CastInst& instruction, ScalarLowering& scalar, IrBuilder& builder) {
    auto* operand = instruction.getOperand(0);
    auto* source = operand->getType();
    auto* target = instruction.getType();
    auto& value = scalar.Get(*operand);
    switch (instruction.getOpcode()) {
        case LLVMBC::Instruction::FPExt:
            if (!IsHalf(*operand) || target->getTypeID() != LLVMBC::Type::TypeID::FloatTyID) throw std::runtime_error("DXIL half extension requires a half source and a float32 result");
            return builder.Emit(IrOpcode::ConvertF32F16, IrType::F32, {&value});
        case LLVMBC::Instruction::FPTrunc:
            if (source->getTypeID() != LLVMBC::Type::TypeID::FloatTyID || !IsHalf(instruction)) throw std::runtime_error("DXIL half truncation requires a float32 source and a half result");
            return builder.Emit(IrOpcode::ConvertF16F32, IrType::F16, {&value});
        case LLVMBC::Instruction::BitCast:
            if (IsHalf(*operand) && target->isIntegerTy() && target->getIntegerBitWidth() == 16) return builder.BitCastU32FromF16(value);
            if (source->isIntegerTy() && source->getIntegerBitWidth() == 16 && IsHalf(instruction)) return builder.BitCastF16(value);
            throw std::runtime_error("DXIL half bitcast requires exactly 16 bits");
        default: throw std::runtime_error("DXIL half cast is not implemented");
    }
}
