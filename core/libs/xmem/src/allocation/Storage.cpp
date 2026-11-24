#include "Storage.hpp"
#include "../virtual/WindowsApi.hpp"
#include <algorithm>
#include <limits>
#include <new>
#include <iterator>

namespace {
bool Dedicated(std::uint64_t attributes) {
    return (attributes & (std::uint64_t{1} << 33)) != 0;
}
}

void* AnyBox::XMem::AllocateHeapStorage(std::size_t size, std::uint64_t attributes, std::size_t alignment) {
    if (!Dedicated(attributes)) return ::operator new(size, std::align_val_t{alignment});
    constexpr std::uint32_t protections[]{PAGE_READWRITE, PAGE_READWRITE | PAGE_WRITECOMBINE, PAGE_READWRITE | PAGE_WRITECOMBINE, PAGE_READWRITE, PAGE_READWRITE, PAGE_READWRITE | PAGE_WRITECOMBINE, PAGE_READWRITE, PAGE_READWRITE, PAGE_READWRITE | PAGE_WRITECOMBINE, PAGE_READWRITE | PAGE_NOCACHE};
    auto kind = static_cast<std::size_t>((attributes >> 29) & 15);
    if (kind >= std::size(protections)) throw std::invalid_argument("Invalid XMem dedicated memory type");
    auto pageMode = (attributes >> 14) & 3;
    auto pageSize = pageMode == 2 || alignment == 131072 ? std::size_t{2097152} : kind != 0 || pageMode != 0 ? std::size_t{65536} : std::size_t{4096};
    if (size > std::numeric_limits<std::size_t>::max() - (pageSize - 1)) throw std::bad_alloc();
    size = (size + pageSize - 1) & ~(pageSize - 1);
    MEM_ADDRESS_REQUIREMENTS requirements{};
    requirements.HighestEndingAddress = reinterpret_cast<void*>(0xFFFFFFFFULL);
    requirements.Alignment = std::max<std::size_t>(65536, alignment);
    MEM_EXTENDED_PARAMETER parameter{};
    parameter.Type = MemExtendedParameterAddressRequirements;
    parameter.Pointer = &requirements;
    static const auto allocate = GetMemoryFunction<AllocateVirtual>("VirtualAlloc2");
    auto* result = allocate(nullptr, nullptr, size, MEM_RESERVE | MEM_COMMIT, protections[kind], &parameter, 1);
    if (!result) Windows::ThrowLastError("VirtualAlloc2 XMem dedicated storage");
    return result;
}

void AnyBox::XMem::FreeHeapStorage(void* allocation, std::uint64_t attributes, std::size_t alignment) {
    if (Dedicated(attributes)) {
        if (!VirtualFree(allocation, 0, MEM_RELEASE)) Windows::ThrowLastError("VirtualFree XMem dedicated storage");
    } else ::operator delete(allocation, std::align_val_t{alignment});
}
