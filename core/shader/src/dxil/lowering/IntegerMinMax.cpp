#include "IntegerExtrema.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <array>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerIntegerExtrema(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (instruction.getNumArgOperands() != 3 || !word(instruction)) throw std::runtime_error("DXIL integer binary intrinsic has an invalid signature");
    for (unsigned index = 0; index < 3; ++index) {
        if (!word(*instruction.getArgOperand(index))) throw std::runtime_error("DXIL integer binary intrinsic requires 32-bit integers");
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode) throw std::runtime_error("DXIL integer binary intrinsic opcode must be constant");
    auto mode = opcode->getUniqueInteger().getZExtValue();
    if (mode < 37 || mode > 40) throw std::runtime_error("DXIL integer binary intrinsic opcode is not implemented");

    auto& left = scalar.Get(*instruction.getArgOperand(1));
    auto& right = scalar.Get(*instruction.getArgOperand(2));
    constexpr std::array operations{IrOpcode::SMax32, IrOpcode::SMin32, IrOpcode::UMax32, IrOpcode::UMin32};
    return builder.Emit(operations.at(mode - 37), IrType::U32, {&left, &right});
}
