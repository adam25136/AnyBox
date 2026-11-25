#include "IntegerExtrema.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <array>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerIntegerExtrema(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, unsigned bits) {
    auto word = [](const LLVMBC::Value& value, unsigned width) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == width; };
    if (bits != 32 && bits != 64) throw std::invalid_argument("DXIL integer binary intrinsic width is unsupported");
    if (instruction.getNumArgOperands() != 3 || !word(instruction, bits)) throw std::runtime_error("DXIL integer binary intrinsic has an invalid signature");
    for (unsigned index = 0; index < 3; ++index) {
        if (!word(*instruction.getArgOperand(index), index == 0 ? 32 : bits)) throw std::runtime_error("DXIL integer binary intrinsic has invalid operand widths");
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode) throw std::runtime_error("DXIL integer binary intrinsic opcode must be constant");
    auto mode = opcode->getUniqueInteger().getZExtValue();
    if (mode < 37 || mode > 40) throw std::runtime_error("DXIL integer binary intrinsic opcode is not implemented");

    auto& left = scalar.Get(*instruction.getArgOperand(1));
    auto& right = scalar.Get(*instruction.getArgOperand(2));
    if (bits == 64) {
        auto comparison = mode < 39 ? IrOpcode::SLessThan64 : IrOpcode::ULessThan64;
        auto& less = builder.Emit(comparison, IrType::Bool, {&left, &right});
        auto* first = mode == 38 || mode == 40 ? &left : &right;
        auto* second = first == &left ? &right : &left;
        auto& low = builder.Select(less, builder.CompositeExtract(*first, 0), builder.CompositeExtract(*second, 0));
        auto& high = builder.Select(less, builder.CompositeExtract(*first, 1), builder.CompositeExtract(*second, 1));
        return builder.ConstructU64(low, high);
    }
    constexpr std::array operations{IrOpcode::SMax32, IrOpcode::SMin32, IrOpcode::UMax32, IrOpcode::UMin32};
    return builder.Emit(operations.at(mode - 37), IrType::U32, {&left, &right});
}
