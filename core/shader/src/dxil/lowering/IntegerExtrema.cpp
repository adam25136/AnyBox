#include "IntegerExtrema.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <array>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerUnsignedExtrema3(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, unsigned expectedOpcode) {
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    if (expectedOpcode < 14 || expectedOpcode > 16) throw std::invalid_argument("Xbox unsigned three-operand extrema opcode is unsupported");
    if (instruction.getNumArgOperands() != 4 || !word(instruction)) throw std::runtime_error("Xbox unsigned three-operand extrema has an invalid signature");
    for (unsigned index = 0; index < 4; ++index) {
        if (!word(*instruction.getArgOperand(index))) throw std::runtime_error("Xbox unsigned three-operand extrema requires 32-bit integers");
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != expectedOpcode) throw std::runtime_error("Xbox unsigned three-operand extrema opcode does not match its declaration");

    auto& first = scalar.Get(*instruction.getArgOperand(1));
    auto& second = scalar.Get(*instruction.getArgOperand(2));
    auto& third = scalar.Get(*instruction.getArgOperand(3));
    constexpr std::array operations{IrOpcode::UMinTri32, IrOpcode::UMaxTri32, IrOpcode::UMedTri32};
    return builder.Emit(operations.at(expectedOpcode - 14), IrType::U32, {&first, &second, &third});
}
