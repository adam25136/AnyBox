#include "AtomicAttributes.hpp"
#include <stdexcept>

const AnyBox::Shader::Dxil::Bitcode::AtomicAttributes& AnyBox::Shader::Dxil::Bitcode::GetAtomicAttributes(const LLVMBC::AtomicRMWInst& instruction) {
    auto found = Detail::atomicAttributes.find(&instruction);
    if (found == Detail::atomicAttributes.end()) throw std::runtime_error("DXIL atomic memory attributes are unavailable");
    return found->second;
}
