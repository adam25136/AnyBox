#include "Words.hpp"
#include "../Scalar.hpp"
#include <cast.hpp>
#include <stdexcept>
#include <string>

std::array<ShaderRecompiler::IrValue*, 2> AnyBox::Shader::Dxil::ReadWordPair(const LLVMBC::Value& source, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder) {
    auto* type = source.getType();
    bool structure = type && type->getTypeID() == LLVMBC::Type::TypeID::StructTyID;
    bool vectorType = type && type->getTypeID() == LLVMBC::Type::TypeID::VectorTyID;
    if ((!structure && !vectorType) || (structure ? type->getStructNumElements() : type->getVectorNumElements()) != 2) throw std::runtime_error("Xbox word pair requires two 32-bit integer members");
    for (unsigned index = 0; index < 2; ++index) {
        auto* member = structure ? type->getStructElementType(index) : type->getVectorElementType();
        if (!member || !member->isIntegerTy() || member->getIntegerBitWidth() != 32) throw std::runtime_error("Xbox word pair member must be a 32-bit integer");
    }
    auto* vector = LLVMBC::dyn_cast<LLVMBC::ConstantDataVector>(&source);
    auto* array = LLVMBC::dyn_cast<LLVMBC::ConstantDataArray>(&source);
    auto* aggregate = LLVMBC::dyn_cast<LLVMBC::ConstantAggregate>(&source);
    auto* composite = LLVMBC::dyn_cast<LLVMBC::CompositeConstructInst>(&source);
    bool zero = LLVMBC::isa<LLVMBC::ConstantAggregateZero>(&source);
    if (LLVMBC::isa<LLVMBC::CallInst>(&source)) {
        auto& value = scalar.Get(source);
        if (value.Type() != ShaderRecompiler::IrType::Vec2U32) throw std::runtime_error("Xbox word pair call returned an incompatible IR type");
        return {&builder.CompositeExtract(value, 0), &builder.CompositeExtract(value, 1)};
    }
    if (vector && vector->getNumElements() != 2) throw std::runtime_error("Xbox constant word pair has an invalid element count");
    if (array && array->getNumElements() != 2) throw std::runtime_error("Xbox constant array word pair has an invalid element count");
    if (aggregate && aggregate->getNumOperands() != 2) throw std::runtime_error("Xbox aggregate word pair has an invalid element count");
    if (composite && composite->getNumOperands() != 2) throw std::runtime_error("Xbox constructed word pair has an invalid element count");
    if (!vector && !array && !aggregate && !composite && !zero) throw std::runtime_error("Xbox word pair expression is not implemented: kind=" + std::to_string(static_cast<unsigned>(source.get_value_kind())));
    std::array<ShaderRecompiler::IrValue*, 2> result{};
    for (unsigned index = 0; index < 2; ++index) {
        const auto* element = vector ? vector->getElementAsConstant(index) : array ? array->getElementAsConstant(index) : aggregate ? aggregate->getOperand(index) : composite ? composite->getOperand(index) : nullptr;
        if (!element && !zero) throw std::runtime_error("Xbox word pair contains an absent element");
        auto* expected = structure ? type->getStructElementType(index) : type->getVectorElementType();
        if (element && element->getType() != expected) throw std::runtime_error("Xbox word pair element type does not match its declaration");
        result[index] = element ? &scalar.Get(*element) : &builder.Constant(0);
    }
    return result;
}
