#include "LaneSwizzle.hpp"
#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerLaneSwizzle(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program) {
    auto integer = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 3 || !integer(instruction)) throw std::runtime_error("Xbox lane swizzle intrinsic has an invalid signature");
    for (unsigned index = 0; index < 3; ++index) if (!integer(*instruction.getArgOperand(index))) throw std::runtime_error("Xbox lane swizzle intrinsic requires 32-bit integer operands");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 53) throw std::runtime_error("Xbox lane swizzle intrinsic opcode is invalid");
    auto* pattern = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(2));
    if (!pattern || pattern->getUniqueInteger().getZExtValue() > UINT16_MAX) throw std::runtime_error("Xbox lane swizzle requires a constant 16-bit pattern");
    auto control = static_cast<std::uint32_t>(pattern->getUniqueInteger().getZExtValue());
    if (control >= 0xC000) throw std::runtime_error("Xbox lane swizzle FFT and rotate modes are not implemented");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("Xbox lane swizzle requires a 32-lane or 64-lane wave");
    auto& input = scalar.Get(*instruction.getArgOperand(1));
    return builder.Emit(IrOpcode::SwizzleU32, IrType::U32, {&input, &builder.Constant(control), &builder.ConstantBool(true)});
}
