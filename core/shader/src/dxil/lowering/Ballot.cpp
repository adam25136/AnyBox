#include "Ballot.hpp"
#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerBallot(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program, bool dxil) {
    auto integer = [](const LLVMBC::Type* type, unsigned bits) { return type && type->isIntegerTy() && type->getIntegerBitWidth() == bits; };
    const auto* result = instruction.getType();
    auto words = dxil ? 4u : 2u;
    if (instruction.getNumArgOperands() != 2 || !result || result->getTypeID() != LLVMBC::Type::TypeID::StructTyID || result->getStructNumElements() != words) throw std::runtime_error("Ballot has an invalid signature");
    for (unsigned index = 0; index < words; ++index) if (!integer(result->getStructElementType(index), 32)) throw std::runtime_error("Ballot result requires 32-bit words");
    if (!integer(instruction.getArgOperand(0)->getType(), 32) || !integer(instruction.getArgOperand(1)->getType(), dxil ? 1u : 32u)) throw std::runtime_error("Ballot opcode or predicate type is invalid");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != (dxil ? 116u : 25u)) throw std::runtime_error("Ballot opcode does not match its declaration");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("Ballot requires a 32-lane or 64-lane wave");

    auto& source = scalar.Get(*instruction.getArgOperand(1));
    auto& predicate = dxil ? source : builder.INotEqual(source, builder.Constant(0));
    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&predicate});
    if (dxil) return ballot;
    return builder.Emit(IrOpcode::CompositeConstructU32x2, IrType::Vec2U32, {&builder.CompositeExtract(ballot, 0), &builder.CompositeExtract(ballot, 1)});
}
