#include "Local.hpp"
#include <cast.hpp>
#include <limits>

AnyBox::Shader::Dxil::LocalLowering::Allocation AnyBox::Shader::Dxil::LocalLowering::sharedAllocation(const LLVMBC::GlobalVariable& variable) {
    auto* pointer = variable.getType();
    if (!pointer || pointer->getTypeID() != LLVMBC::Type::TypeID::PointerTyID || pointer->getPointerAddressSpace() != 3 || variable.isConstant()) throw std::runtime_error("DXIL global memory requires a writable groupshared variable");
    if (variable.hasInitializer() && !LLVMBC::isa<LLVMBC::UndefValue>(variable.getInitializer())) throw std::runtime_error("DXIL groupshared initializers are not implemented");
    auto bytes = size(*pointer->getPointerElementType());
    auto base = program.Info().sharedMemoryBytes;
    auto end = (static_cast<std::uint64_t>(base) + bytes + 3) & ~std::uint64_t{3};
    if (end > std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL groupshared allocations exceed the native address range");
    Allocation allocation{base, bytes, true};
    allocations.emplace(&variable, allocation);
    program.Info().sharedMemoryBytes = static_cast<std::uint32_t>(end);
    return allocation;
}
