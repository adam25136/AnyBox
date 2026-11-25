#include "Local.hpp"
#include <cast.hpp>
#include <type.hpp>
#include <limits>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::LocalLowering;

LocalLowering::LocalLowering(IrProgram& program, IrBuilder& builder) : program(program), builder(builder) {}

std::uint32_t LocalLowering::size(const LLVMBC::Type& type) {
    if (type.isIntegerTy() && type.getIntegerBitWidth() == 64) return 8;
    if (type.getTypeID() == LLVMBC::Type::TypeID::FloatTyID || (type.isIntegerTy() && type.getIntegerBitWidth() == 32)) return 4;
    if (type.getTypeID() != LLVMBC::Type::TypeID::ArrayTyID) throw std::runtime_error("DXIL local storage type is not implemented");
    auto count = type.getArrayNumElements();
    auto bytes = size(*type.getArrayElementType());
    if (count == 0 || count > std::numeric_limits<std::uint32_t>::max() / bytes) throw std::runtime_error("DXIL local array size is invalid");
    return static_cast<std::uint32_t>(count * bytes);
}

void LocalLowering::Prepare(const LLVMBC::Function& function) {
    if (!allocations.empty()) throw std::logic_error("DXIL local storage is already prepared");
    for (auto& block : function) {
        for (auto& instruction : block) {
            auto* allocated = LLVMBC::dyn_cast<LLVMBC::AllocaInst>(&instruction);
            if (!allocated) continue;
            if (&block != &function.getEntryBlock()) throw std::runtime_error("DXIL non-entry local allocation is not implemented");
            auto* pointer = allocated->getType();
            if (!pointer || pointer->getTypeID() != LLVMBC::Type::TypeID::PointerTyID || pointer->getPointerAddressSpace() != 0) throw std::runtime_error("DXIL local allocation requires an address-space-zero pointer");
            auto* count = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(allocated->getArraySize());
            if (!count || !count->getType()->isIntegerTy() || (count->getType()->getIntegerBitWidth() != 32 && count->getType()->getIntegerBitWidth() != 64)) throw std::runtime_error("DXIL dynamic local allocation is not implemented");
            auto elements = count->getUniqueInteger().getZExtValue();
            auto elementBytes = size(*pointer->getPointerElementType());
            auto base = static_cast<std::uint64_t>(program.Info().scratchDwords) * 4;
            if (elements == 0 || base > std::numeric_limits<std::uint32_t>::max() || elements > (std::numeric_limits<std::uint32_t>::max() - base) / elementBytes) throw std::runtime_error("DXIL local allocation exceeds the native address range");
            auto bytes = static_cast<std::uint32_t>(elements * elementBytes);
            allocations.emplace(allocated, Allocation{static_cast<std::uint32_t>(base), bytes});
            program.Info().scratchDwords += bytes / 4;
        }
    }
    preparePrivateGlobals(function);
}
