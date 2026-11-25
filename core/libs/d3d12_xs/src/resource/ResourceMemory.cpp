#include "ResourceMemory.hpp"
#include <xmem.hpp>
#include <bit>
#include <limits>
#include <utility>
#include <stdexcept>
#include <new>

AnyBox::Graphics::ResourceMemory::ResourceMemory(std::uint64_t size, std::uint64_t alignment, D3D12_HEAP_FLAGS flags, std::uint32_t memoryType) : size(size), attributes(0) {
    if (memoryType > 9) throw std::invalid_argument("Resource allocation memory type is invalid");
    if (size == 0 || size > std::numeric_limits<std::size_t>::max() || !std::has_single_bit(alignment) || alignment < 4096 || alignment > 2097152) throw std::invalid_argument("Resource allocation size or alignment is invalid");
    if ((static_cast<std::uint32_t>(flags) & ~static_cast<std::uint32_t>(D3D12_HEAP_FLAG_ALLOW_DISPLAY)) != 0) throw std::runtime_error("Resource allocation heap flags are not implemented");
    bool display = (flags & D3D12_HEAP_FLAG_ALLOW_DISPLAY) != 0;
    auto pageMode = display || alignment > 65536 ? 2ULL : 1ULL;
    auto exponent = static_cast<std::uint64_t>(std::countr_zero(alignment));
    attributes = (1ULL << 33) | (static_cast<std::uint64_t>(memoryType) << 29) | (exponent << 24) | (pageMode << 14) | (display ? 0x800013 : 0x800004);
    address = XMemAlloc(static_cast<std::size_t>(size), attributes);
    if (!address) throw std::bad_alloc();
    if (reinterpret_cast<std::uintptr_t>(address) % alignment != 0 || reinterpret_cast<std::uintptr_t>(address) % 65536 != 0) {
        XMemFree(address, attributes);
        throw std::invalid_argument("Resource allocation hook returned a misaligned address");
    }
}

AnyBox::Graphics::ResourceMemory::ResourceMemory(ResourceMemory&& other) noexcept : address(std::exchange(other.address, nullptr)), size(other.size), attributes(other.attributes) {}

AnyBox::Graphics::ResourceMemory::~ResourceMemory() {
    if (address) XMemFree(address, attributes);
}
