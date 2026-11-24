#include <anybox/shader/runtime/MemoryTable.hpp>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

using namespace AnyBox::Shader::Runtime;
namespace Abi = ShaderRecompiler::BdaAbi;

MemoryTable::MemoryTable(std::span<const MemoryBinding> bindings) : bindings(bindings.begin(), bindings.end()) {
    static_assert(std::endian::native == std::endian::little);
    for (const auto& binding : bindings) {
        if (!binding.mapping || binding.permissions == 0 || (binding.permissions & ~(Abi::Read | Abi::Write)) != 0) throw std::invalid_argument("GPU memory table binding is invalid");
    }
    std::stable_sort(this->bindings.begin(), this->bindings.end(), [](const auto& left, const auto& right) {
        auto begin = left.mapping->GetGuestAddress();
        auto other = right.mapping->GetGuestAddress();
        if (begin != other) return begin < other;
        return left.mapping->GetSize() > right.mapping->GetSize();
    });
    ranges.reserve(bindings.size());
    std::size_t retained = 0;
    for (const auto& binding : this->bindings) {
        const auto& mapping = *binding.mapping;
        auto begin = mapping.GetGuestAddress();
        auto end = begin + mapping.GetSize();
        if (!ranges.empty() && begin < ranges.back().end) {
            if (binding.permissions != ranges.back().permissions) throw std::runtime_error("Overlapping GPU mappings with different permissions are not implemented");
            if (end <= ranges.back().end) continue;
        }
        ranges.push_back({begin, end, mapping.GetDeviceAddress(), binding.permissions, 0});
        this->bindings[retained++] = binding;
    }
    this->bindings.resize(retained);
    if (ranges.size() > (std::numeric_limits<std::uint32_t>::max() - 4) / 8) throw std::length_error("GPU memory table exceeds the shader ABI capacity");
}

std::size_t MemoryTable::GetByteSize() const noexcept { return sizeof(Abi::Header) + ranges.size() * sizeof(Abi::Range); }

void MemoryTable::Write(std::span<std::byte> destination) const {
    if (destination.size() != GetByteSize()) throw std::invalid_argument("GPU memory table destination size is invalid");
    Abi::Header header{Abi::Version, static_cast<std::uint32_t>(ranges.size()), sizeof(Abi::Range), 0};
    std::memcpy(destination.data(), &header, sizeof(header));
    if (!ranges.empty()) std::memcpy(destination.data() + sizeof(header), ranges.data(), ranges.size() * sizeof(Abi::Range));
}
