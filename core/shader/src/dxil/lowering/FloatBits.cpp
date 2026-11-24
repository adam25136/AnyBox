#include "FloatBits.hpp"
#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader::Dxil;

IrValue& AnyBox::Shader::Dxil::FlushFloat32(IrBuilder& builder, IrValue& value) {
    auto& bits = builder.BitCastU32(value);
    auto& exponent = builder.BitwiseAnd(bits, builder.Constant(0x7F800000U));
    auto& zeroExponent = builder.IEqual(exponent, builder.Constant(0));
    auto& sign = builder.BitwiseAnd(bits, builder.Constant(0x80000000U));
    return builder.BitCastF32(builder.Select(zeroExponent, sign, bits));
}

IrValue& AnyBox::Shader::Dxil::LowerUnaryFloat(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder) {
    if (instruction.getNumArgOperands() != 2 || ScalarLowering::GetType(instruction) != IrType::F32 || ScalarLowering::GetType(*instruction.getArgOperand(1)) != IrType::F32) throw std::runtime_error("DXIL floating unary intrinsic has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || !opcode->getType()->isIntegerTy() || opcode->getType()->getIntegerBitWidth() != 32) throw std::runtime_error("DXIL floating unary intrinsic requires a constant 32-bit opcode");
    auto mode = opcode->getUniqueInteger().getZExtValue();
    if (mode != 6 && mode != 7 && mode != 22 && mode != 28) throw std::runtime_error("DXIL floating unary intrinsic mode is not implemented");
    auto& input = scalar.Get(*instruction.getArgOperand(1));
    if (mode == 6) return builder.BitCastF32(builder.BitwiseAnd(builder.BitCastU32(input), builder.Constant(0x7FFFFFFF)));

    auto& value = FlushFloat32(builder, input);
    if (mode == 7) return builder.Emit(IrOpcode::FPSaturate32, IrType::F32, {&value});
    if (mode == 28) return builder.Emit(IrOpcode::FPCeil32, IrType::F32, {&value});
    auto& fraction = builder.Emit(IrOpcode::FPFract32, IrType::F32, {&value});
    auto& upper = builder.BitCastF32(builder.Constant(0x3F7FFFFF));
    auto& exceeds = builder.Emit(IrOpcode::FPOrdGreaterThan32, IrType::Bool, {&fraction, &upper});
    auto& bounded = builder.Select(exceeds, upper, fraction);
    auto& magnitude = builder.BitwiseAnd(builder.BitCastU32(value), builder.Constant(0x7FFFFFFF));
    auto& invalid = builder.UGreaterThan(magnitude, builder.Constant(0x7F7FFFFF));
    return builder.Select(invalid, builder.BitCastF32(builder.Constant(0x7FC00000)), bounded);
}

IrValue& AnyBox::Shader::Dxil::LowerCopySign(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder) {
    if (instruction.getNumArgOperands() != 2 || ScalarLowering::GetType(instruction) != IrType::F32 || ScalarLowering::GetType(*instruction.getArgOperand(0)) != IrType::F32 || ScalarLowering::GetType(*instruction.getArgOperand(1)) != IrType::F32) throw std::runtime_error("LLVM copysign requires two float32 operands and a float32 result");
    auto& value = scalar.Get(*instruction.getArgOperand(0));
    auto& source = scalar.Get(*instruction.getArgOperand(1));
    auto& magnitude = builder.BitwiseAnd(builder.BitCastU32(value), builder.Constant(0x7FFFFFFF));
    auto& sign = builder.BitwiseAnd(builder.BitCastU32(source), builder.Constant(0x80000000));
    return builder.BitCastF32(builder.BitwiseOr(magnitude, sign));
}
