#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::barrier(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 2 || instruction.getType()->getTypeID() != LLVMBC::Type::TypeID::VoidTyID) throw std::runtime_error("DXIL barrier has an invalid signature");
    auto constant = [&](unsigned index) {
        auto* value = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(index));
        if (!value || width(*value) != 32) throw std::runtime_error("DXIL barrier requires 32-bit constant operands");
        return value->getUniqueInteger().getZExtValue();
    };
    if (constant(0) != 80) throw std::runtime_error("DXIL barrier opcode is invalid");
    auto mode = constant(1);
    if (mode != 1 && mode != 9) throw std::runtime_error("DXIL barrier memory mode is not implemented: " + std::to_string(mode));
    return builder.Emit(IrOpcode::Barrier, IrType::Void, {});
}
