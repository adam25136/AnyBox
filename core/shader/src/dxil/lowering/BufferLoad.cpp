#include "Buffer.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <bit>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

void BufferLowering::load(const LLVMBC::CallInst& instruction, unsigned bits, ScalarLowering& scalar, bool floating) {
    if (instruction.getNumArgOperands() != 6) throw std::runtime_error("DXIL raw buffer load has an invalid argument count");
    if (constant(instruction, 0, 32) != 139) throw std::runtime_error("DXIL raw buffer load opcode is invalid");
    auto mask = constant(instruction, 4, 8);
    auto alignment = constant(instruction, 5, 32);
    if (mask == 0 || mask > 15 || !std::has_single_bit(alignment)) throw std::runtime_error("DXIL raw buffer load mask or alignment is invalid");
    auto* result = instruction.getType();
    if (!result || result->getTypeID() != LLVMBC::Type::TypeID::StructTyID || result->getStructNumElements() != 5) throw std::runtime_error("DXIL raw buffer load result must contain five members");
    for (unsigned index = 0; index < 5; ++index) {
        auto* member = result->getStructElementType(index);
        auto floatingType = bits == 16 ? LLVMBC::Type::TypeID::HalfTyID : LLVMBC::Type::TypeID::FloatTyID;
        bool valid = floating && index < 4 ? member->getTypeID() == floatingType : member->isIntegerTy() && member->getIntegerBitWidth() == (index == 4 ? 32 : bits);
        if (!valid) throw std::runtime_error("DXIL raw buffer load result member type is invalid");
    }
    auto access = resolve(instruction, scalar, false);
    std::array<IrValue*, 5> output{};
    for (unsigned component = 0; component < 4; ++component) {
        if ((mask & (1u << component)) == 0) continue;
        auto part = this->component(access, bits, component);
        output[component] = &read(part.address, part.active, bits, access.binding.globallyCoherent);
        if (floating) output[component] = bits == 16 ? &builder.BitCastF16(*output[component]) : &builder.BitCastF32(*output[component]);
    }
    aggregates.emplace(&instruction, output);
}
