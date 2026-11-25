#include "Scalar.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::globalIntegerAtomic(const LLVMBC::CallInst& instruction, bool subtract) {
    if (instruction.getNumArgOperands() != 3 || width(instruction) != 32) throw std::runtime_error("Xbox integer global atomic has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || width(*instruction.getArgOperand(1)) != 64 || width(*instruction.getArgOperand(2)) != 32) throw std::runtime_error("Xbox integer global atomic has invalid operand widths");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != (subtract ? 367u : 355u)) throw std::runtime_error("Xbox integer global atomic opcode does not match its declaration");
    auto& address = Get(*instruction.getArgOperand(1));
    auto& value = Get(*instruction.getArgOperand(2));
    return globalAtomic(subtract ? IrOpcode::AddressAtomicISub32 : IrOpcode::AddressAtomicIAdd32, address, value);
}
