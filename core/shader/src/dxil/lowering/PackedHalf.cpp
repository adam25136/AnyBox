#include "HalfExtrema.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <array>

namespace {
using namespace ShaderRecompiler;
using namespace AnyBox::Shader::Dxil;

bool HalfPair(const LLVMBC::Type* type) {
    if (!type || type->getTypeID() != LLVMBC::Type::TypeID::StructTyID || type->getStructNumElements() != 2) return false;
    for (unsigned index = 0; index < 2; ++index) if (type->getStructElementType(index)->getTypeID() != LLVMBC::Type::TypeID::HalfTyID) return false;
    return true;
}

std::array<IrValue*, 2> ReadPair(const LLVMBC::Value& value, ScalarLowering& scalar, IrBuilder& builder) {
    if (!HalfPair(value.getType())) throw std::runtime_error("Xbox packed half min/max requires a pair of half values");
    auto* composite = LLVMBC::dyn_cast<LLVMBC::CompositeConstructInst>(&value);
    auto* aggregate = LLVMBC::dyn_cast<LLVMBC::ConstantAggregate>(&value);
    auto* data = LLVMBC::dyn_cast<LLVMBC::ConstantDataArray>(&value);
    bool zero = LLVMBC::isa<LLVMBC::UndefValue>(&value) || LLVMBC::isa<LLVMBC::ConstantAggregateZero>(&value);
    if ((composite && composite->getNumOperands() != 2) || (aggregate && aggregate->getNumOperands() != 2) || (data && data->getNumElements() != 2)) throw std::runtime_error("Xbox packed half min/max pair has an invalid element count");
    std::array<IrValue*, 2> result{};
    if (!composite && !aggregate && !data && !zero) {
        auto& pair = scalar.Get(value);
        if (!IsVectorType(pair.Type()) || ComponentCount(pair.Type()) != 2) throw std::runtime_error("Xbox packed half min/max pair has an incompatible IR representation");
        for (unsigned index = 0; index < 2; ++index) result[index] = &builder.CompositeExtract(pair, index);
        return result;
    }
    for (unsigned index = 0; index < 2; ++index) {
        auto* input = composite ? composite->getOperand(index) : aggregate ? aggregate->getOperand(index) : data ? data->getElementAsConstant(index) : nullptr;
        if (input && input->getType() != value.getType()->getStructElementType(index)) throw std::runtime_error("Xbox packed half min/max pair element type differs from its declaration");
        result[index] = input ? &builder.BitCastU32FromF16(scalar.Get(*input)) : &builder.Constant(0);
    }
    return result;
}
}

ShaderRecompiler::IrValue& AnyBox::Shader::Dxil::LowerPackedHalfExtremum(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, bool maximum) {
    if (instruction.getNumArgOperands() != 3 || !HalfPair(instruction.getType())) throw std::runtime_error("Xbox packed half min/max has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || !opcode->getType()->isIntegerTy() || opcode->getType()->getIntegerBitWidth() != 32 || opcode->getUniqueInteger().getZExtValue() != (maximum ? 224u : 223u)) throw std::runtime_error("Xbox packed half min/max opcode is invalid");
    auto left = ReadPair(*instruction.getArgOperand(1), scalar, builder);
    auto right = ReadPair(*instruction.getArgOperand(2), scalar, builder);
    auto& low = HalfExtremumBits(builder, *left[0], *right[0], maximum);
    auto& high = HalfExtremumBits(builder, *left[1], *right[1], maximum);
    return builder.Emit(IrOpcode::CompositeConstructU32x2, IrType::Vec2U32, {&low, &high});
}
