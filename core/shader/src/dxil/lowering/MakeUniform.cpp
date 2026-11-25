#include "MakeUniform.hpp"
#include "Scalar.hpp"
#include "AddressLoad.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerMakeUniform(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, IrProgram& program, unsigned bits) {
    auto integer = [](const LLVMBC::Value& value, unsigned width) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == width; };
    if (bits != 32 && bits != 64) throw std::invalid_argument("Xbox uniform intrinsic width is unsupported");
    if (instruction.getNumArgOperands() != 2 || !integer(instruction, bits)) throw std::runtime_error("Xbox uniform intrinsic has an invalid signature");
    if (!integer(*instruction.getArgOperand(0), 32) || !integer(*instruction.getArgOperand(1), bits)) throw std::runtime_error("Xbox uniform intrinsic has invalid operand widths");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 50) throw std::runtime_error("Xbox uniform intrinsic opcode does not match its declaration");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("Xbox uniform intrinsic requires a 32-lane or 64-lane wave");

    auto& value = scalar.Get(*instruction.getArgOperand(1));
    if (IsUniformAddress(value, program)) return value;
    auto first = [&](IrValue& input) -> IrValue& { return builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&input, &builder.ConstantBool(true)}); };
    if (bits == 32) return first(value);
    auto& low = first(builder.CompositeExtract(value, 0));
    auto& high = first(builder.CompositeExtract(value, 1));
    return builder.ConstructU64(low, high);
}
