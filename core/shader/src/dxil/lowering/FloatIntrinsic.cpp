#include "Scalar.hpp"
#include "FloatBits.hpp"
#include <cast.hpp>
#include <array>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

using AnyBox::Shader::Dxil::FlushFloat32;

IrValue& ScalarLowering::floatBinaryIntrinsic(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 3 || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL floating binary intrinsic has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || GetType(*instruction.getArgOperand(1)) != IrType::F32 || GetType(*instruction.getArgOperand(2)) != IrType::F32) throw std::runtime_error("DXIL floating binary intrinsic has invalid operand types");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode) throw std::runtime_error("DXIL floating binary intrinsic opcode must be constant");
    IrOpcode operation;
    switch (opcode->getUniqueInteger().getZExtValue()) {
        case 35: operation = IrOpcode::FPMax32; break;
        case 36: operation = IrOpcode::FPMin32; break;
        default: throw std::runtime_error("DXIL floating binary intrinsic opcode is not implemented");
    }
    auto& left = FlushFloat32(builder, Get(*instruction.getArgOperand(1)));
    auto& right = FlushFloat32(builder, Get(*instruction.getArgOperand(2)));
    return builder.Emit(operation, IrType::F32, {&left, &right});
}

IrValue& ScalarLowering::floatTernaryIntrinsic(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 4 || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL floating ternary intrinsic has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32) throw std::runtime_error("DXIL floating ternary intrinsic requires a 32-bit opcode");
    for (unsigned index = 1; index < 4; ++index) {
        if (GetType(*instruction.getArgOperand(index)) != IrType::F32) throw std::runtime_error("DXIL floating ternary intrinsic requires 32-bit floating operands");
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 46) throw std::runtime_error("DXIL floating ternary intrinsic opcode is not implemented");

    auto& left = Get(*instruction.getArgOperand(1));
    auto& right = Get(*instruction.getArgOperand(2));
    auto& addend = Get(*instruction.getArgOperand(3));
    return builder.Emit(IrOpcode::FPMad32, IrType::F32, {&left, &right, &addend});
}

IrValue& ScalarLowering::floatExtrema3(const LLVMBC::CallInst& instruction, unsigned expectedOpcode) {
    if (instruction.getNumArgOperands() != 4 || GetType(instruction) != IrType::F32) throw std::runtime_error("Xbox three-operand extrema has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || width(*opcode) != 32 || opcode->getUniqueInteger().getZExtValue() != expectedOpcode) throw std::runtime_error("Xbox three-operand extrema opcode does not match its declaration");
    for (unsigned index = 1; index < 4; ++index) {
        if (GetType(*instruction.getArgOperand(index)) != IrType::F32) throw std::runtime_error("Xbox three-operand extrema requires float32 operands");
    }

    auto& first = FlushFloat32(builder, Get(*instruction.getArgOperand(1)));
    auto& second = FlushFloat32(builder, Get(*instruction.getArgOperand(2)));
    auto& third = FlushFloat32(builder, Get(*instruction.getArgOperand(3)));
    constexpr std::array operations{IrOpcode::FPMinTri32, IrOpcode::FPMaxTri32, IrOpcode::FPMedTri32};
    return builder.Emit(operations.at(expectedOpcode - 8), IrType::F32, {&first, &second, &third});
}
