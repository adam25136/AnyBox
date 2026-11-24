#include <anybox/shader/runtime/RootResources.hpp>
#include <algorithm>
#include <cstring>
#include <stdexcept>

using namespace AnyBox::Shader;
using Runtime::RootResources;

RootBufferValue RootResources::ResolveTable(const BufferBinding& binding, std::uint32_t tableAddress, std::uint32_t descriptorOffset) {
    checkBinding(binding);
    if (tableAddress == 0 || tableAddress % 16 != 0) throw std::invalid_argument("Compute descriptor table address is null or misaligned");
    auto location = static_cast<std::uint64_t>(tableAddress) + static_cast<std::uint64_t>(descriptorOffset) * 32;
    if (location > UINT32_MAX - 31) throw std::out_of_range("Compute descriptor table offset exceeds its address encoding");
    auto table = tables.Resolve(location, 32);
    std::array<std::uint32_t, 8> words;
    std::memcpy(words.data(), reinterpret_cast<const void*>(static_cast<std::uintptr_t>(location)), sizeof(words));
    RootBufferValue value;

    if (binding.kind == BufferKind::Constant) {
        if ((words[1] >> 16) != 0x10 || words[3] != 0x2201403C) throw std::runtime_error("Compute constant buffer descriptor encoding is not implemented");
        auto address = words[0] | (static_cast<std::uint64_t>(words[1] & 0xFFFF) << 32);
        auto length = static_cast<std::uint64_t>(words[2]) * 16;
        value = address == nullConstantAddress ? RootBufferValue{0, 0} : resolveView(address, length);
    } else if (std::ranges::all_of(words, [](auto word) { return word == 0; })) value = {0, 0};
    else {
        auto swizzle = binding.resourceClass == ResourceClass::UnorderedAccess ? 0x204u : 0xFACu;
        if (binding.kind != BufferKind::Raw || (words[1] >> 16) != 0 || words[3] != (0x30014000 | swizzle) || !std::ranges::all_of(std::span(words).subspan(4), [](auto word) { return word == 0; })) throw std::runtime_error("Compute shader buffer descriptor encoding is not implemented");
        auto address = words[0] | (static_cast<std::uint64_t>(words[1]) << 32);
        value = resolveView(address, words[2]);
    }
    retain(std::move(table));
    return value;
}
