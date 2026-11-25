#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>

namespace LLVMBC { class AtomicRMWInst; }

namespace AnyBox::Shader::Dxil::Bitcode {
struct AtomicAttributes {
    bool volatileAccess;
    std::uint32_t ordering;
    std::uint32_t scope;
};

namespace Detail {
inline thread_local std::unordered_map<const LLVMBC::AtomicRMWInst*, AtomicAttributes> atomicAttributes;
}

inline void ResetAtomicAttributes() { Detail::atomicAttributes.clear(); }

inline bool StoreAtomicAttributes(const LLVMBC::AtomicRMWInst& instruction, std::span<const std::uint64_t> values) {
    if (values.size() != 3 || values[0] > 1 || values[1] < 2 || values[1] > 6 || values[2] > UINT32_MAX) return false;
    AtomicAttributes attributes{values[0] != 0, static_cast<std::uint32_t>(values[1]), static_cast<std::uint32_t>(values[2])};
    return Detail::atomicAttributes.emplace(&instruction, attributes).second;
}

const AtomicAttributes& GetAtomicAttributes(const LLVMBC::AtomicRMWInst& instruction);
}
