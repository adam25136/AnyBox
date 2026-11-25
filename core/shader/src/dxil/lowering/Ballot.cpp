#include "Ballot.hpp"
#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerBallot64(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, IrProgram& program) {
    auto integer = [](const LLVMBC::Type* type, unsigned bits) { return type && type->isIntegerTy() && type->getIntegerBitWidth() == bits; };
    const auto* result = instruction.getType();
    if (instruction.getNumArgOperands() != 2 || !result || result->getTypeID() != LLVMBC::Type::TypeID::StructTyID || result->getStructNumElements() != 2) throw std::runtime_error("Xbox ballot has an invalid signature");
    if (!integer(result->getStructElementType(0), 32) || !integer(result->getStructElementType(1), 32)) throw std::runtime_error("Xbox ballot result requires two 32-bit words");
    if (!integer(instruction.getArgOperand(0)->getType(), 32) || !integer(instruction.getArgOperand(1)->getType(), 32)) throw std::runtime_error("Xbox ballot requires a 32-bit opcode and integer predicate");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 25) throw std::runtime_error("Xbox ballot opcode does not match its declaration");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("Xbox ballot requires a 32-lane or 64-lane wave");

    auto& predicate = builder.INotEqual(scalar.Get(*instruction.getArgOperand(1)), builder.Constant(0));
    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&predicate});
    return builder.Emit(IrOpcode::CompositeConstructU32x2, IrType::Vec2U32, {&builder.CompositeExtract(ballot, 0), &builder.CompositeExtract(ballot, 1)});
}
