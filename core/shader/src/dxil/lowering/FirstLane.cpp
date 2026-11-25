#include "FirstLane.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;

IrValue& AnyBox::Shader::Dxil::LowerFirstLane(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, IrBuilder& builder, const IrProgram& program, unsigned bits, bool floating) {
    if ((bits != 1 && bits != 32 && bits != 64) || (floating && bits != 32)) throw std::invalid_argument("DXIL first lane overload is unsupported");
    auto* type = instruction.getType();
    bool valid = type && (floating ? type->getTypeID() == LLVMBC::Type::TypeID::FloatTyID : type->isIntegerTy() && type->getIntegerBitWidth() == bits);
    if (!valid || instruction.getNumArgOperands() != 2 || instruction.getArgOperand(1)->getType() != type) throw std::runtime_error("DXIL first lane intrinsic has an invalid signature");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getType()->getIntegerBitWidth() != 32 || opcode->getUniqueInteger().getZExtValue() != 118) throw std::runtime_error("DXIL first lane intrinsic opcode is invalid");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::runtime_error("DXIL first lane intrinsic requires a 32-lane or 64-lane wave");

    auto& value = scalar.Get(*instruction.getArgOperand(1));
    if (value.HasImmediate()) return value;
    auto first = [&](IrValue& input) -> IrValue& { return builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&input, &builder.ConstantBool(true)}); };
    if (bits == 64) {
        auto& low = first(builder.CompositeExtract(value, 0));
        auto& high = first(builder.CompositeExtract(value, 1));
        return builder.ConstructU64(low, high);
    }
    auto& word = bits == 1 ? builder.Select(value, builder.Constant(1), builder.Constant(0)) : floating ? builder.BitCastU32(value) : value;
    auto& result = first(word);
    if (bits == 1) return builder.INotEqual(result, builder.Constant(0));
    return floating ? builder.BitCastF32(result) : result;
}
