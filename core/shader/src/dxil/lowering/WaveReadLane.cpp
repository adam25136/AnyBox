#include "WaveReadLane.hpp"
#include "Scalar.hpp"
#include "LaneReadMode.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerWaveReadLane(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program, unsigned bits, bool floating) {
    if ((bits != 1 && bits != 32 && bits != 64) || (floating && bits != 32)) throw std::invalid_argument("DXIL indexed lane overload is unsupported");
    auto* type = instruction.getType();
    auto word = [](const LLVMBC::Value& value) { return value.getType() && value.getType()->isIntegerTy() && value.getType()->getIntegerBitWidth() == 32; };
    bool valid = type && (floating ? type->getTypeID() == LLVMBC::Type::TypeID::FloatTyID : type->isIntegerTy() && type->getIntegerBitWidth() == bits);
    if (!valid || instruction.getNumArgOperands() != 3 || instruction.getArgOperand(1)->getType() != type || !word(*instruction.getArgOperand(0)) || !word(*instruction.getArgOperand(2))) throw std::runtime_error("DXIL indexed lane intrinsic has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 117) throw std::runtime_error("DXIL indexed lane intrinsic opcode is invalid");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("DXIL indexed lane intrinsic requires a 32-lane or 64-lane wave");
    auto* constant = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(2));
    if (constant && constant->getUniqueInteger().getZExtValue() >= program.WaveSize()) throw std::runtime_error("DXIL indexed lane constant selector is outside its wave");

    auto& value = scalar.Get(*instruction.getArgOperand(1));
    auto& selector = scalar.Get(*instruction.getArgOperand(2));
    if (value.HasImmediate()) return value;
    auto read = [&](IrValue& input) -> IrValue& { return builder.Emit(IrOpcode::ReadLane, IrType::U32, {&input, &selector}, LaneReadMode::Active); };
    if (bits == 64) {
        auto& low = read(builder.CompositeExtract(value, 0));
        auto& high = read(builder.CompositeExtract(value, 1));
        return builder.ConstructU64(low, high);
    }
    auto& encoded = bits == 1 ? builder.Select(value, builder.Constant(1), builder.Constant(0)) : floating ? builder.BitCastU32(value) : value;
    auto& result = read(encoded);
    if (bits == 1) return builder.INotEqual(result, builder.Constant(0));
    return floating ? builder.BitCastF32(result) : result;
}
