#include "Local.hpp"
#include "Scalar.hpp"
#include "../bitcode/AtomicAttributes.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::LocalLowering;

namespace {
IrOpcode Operation(LLVMBC::AtomicRMWInst::BinOp operation) {
    using BinOp = LLVMBC::AtomicRMWInst::BinOp;
    switch (operation) {
        case BinOp::Xchg: return IrOpcode::SharedAtomicSwap32;
        case BinOp::Add: return IrOpcode::SharedAtomicIAdd32;
        case BinOp::Sub: return IrOpcode::SharedAtomicISub32;
        case BinOp::And: return IrOpcode::SharedAtomicAnd32;
        case BinOp::Or: return IrOpcode::SharedAtomicOr32;
        case BinOp::Xor: return IrOpcode::SharedAtomicXor32;
        case BinOp::Max: return IrOpcode::SharedAtomicSMax32;
        case BinOp::Min: return IrOpcode::SharedAtomicSMin32;
        case BinOp::UMax: return IrOpcode::SharedAtomicUMax32;
        case BinOp::UMin: return IrOpcode::SharedAtomicUMin32;
        default: throw std::runtime_error("DXIL shared atomic operation is not implemented");
    }
}
}

IrValue& LocalLowering::Atomic(const LLVMBC::AtomicRMWInst& instruction, ScalarLowering& scalar) {
    auto* type = instruction.getType();
    if (!type || !type->isIntegerTy() || type->getIntegerBitWidth() != 32 || instruction.getValOperand()->getType() != type) throw std::runtime_error("DXIL shared atomic requires matching 32-bit integer types");
    const auto& attributes = Bitcode::GetAtomicAttributes(instruction);
    if (attributes.volatileAccess || (attributes.ordering != 2 && attributes.ordering != 6) || attributes.scope != 1) throw std::runtime_error("DXIL shared atomic memory mode is not implemented: volatile=" + std::to_string(attributes.volatileAccess) + " ordering=" + std::to_string(attributes.ordering) + " scope=" + std::to_string(attributes.scope));
    auto opcode = Operation(instruction.getOperation());
    auto pointer = resolve(*instruction.getPointerOperand(), scalar);
    if (!pointer.allocation.shared || pointer.allocation.constants) throw std::runtime_error("DXIL atomic operation requires writable groupshared memory");
    auto location = access(pointer, *type);
    auto& value = scalar.Get(*instruction.getValOperand());
    return builder.Emit(opcode, IrType::U32, {&location.offset, &value, &location.active}, flags(true));
}
