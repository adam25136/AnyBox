#include "Tests.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <atomic>

void AnyBox::Tests::CheckGraphicsHeap() {
    for (std::uint64_t kind : {1u, 3u, 8u, 9u}) {
        auto attributes = (std::uint64_t{1} << 33) | (kind << 29) | (16ULL << 24) | (1ULL << 14) | 0x800004;
        auto* allocation = static_cast<std::byte*>(XMemAllocDefault(65537, attributes));
        Require(allocation && reinterpret_cast<std::uintptr_t>(allocation) % 65536 == 0, "XMem dedicated allocation is not aligned");
        Require(reinterpret_cast<std::uintptr_t>(allocation) <= UINT32_MAX - 65536, "XMem 32-bit allocation crossed the 4 GiB boundary");
        MEMORY_BASIC_INFORMATION information{};
        if (!VirtualQuery(allocation, &information, sizeof(information))) Windows::ThrowLastError("Query XMem dedicated storage");
        auto protection = kind == 3 ? PAGE_READWRITE : PAGE_READWRITE | (kind == 9 ? PAGE_NOCACHE : PAGE_WRITECOMBINE);
        Require(information.AllocationBase == allocation && information.State == MEM_COMMIT && information.RegionSize >= 131072 && information.Protect == static_cast<DWORD>(protection), "XMem dedicated storage has incorrect virtual memory properties");
        allocation[0] = std::byte{0x31};
        allocation[65536] = std::byte{0x73};
        std::atomic_thread_fence(std::memory_order_seq_cst);
        Require(allocation[0] == std::byte{0x31} && allocation[65536] == std::byte{0x73}, "XMem dedicated storage did not retain data");
        ExpectException<std::invalid_argument>([&] { XMemFreeDefault(allocation, attributes ^ (std::uint64_t{1} << 33)); });
        XMemFreeDefault(allocation, attributes);
        if (!VirtualQuery(allocation, &information, sizeof(information))) Windows::ThrowLastError("Query released XMem dedicated storage");
        Require(information.State == MEM_FREE, "XMem dedicated storage was not released");
    }
    for (std::uint64_t exponent : {0u, 4u, 16u, 20u}) {
        auto attributes = (std::uint64_t{1} << 33) | (exponent << 24);
        auto* allocation = XMemAllocDefault(333, attributes);
        Require(allocation && reinterpret_cast<std::uintptr_t>(allocation) <= UINT32_MAX - 332, "XMem default memory ignored its 32-bit address requirement");
        auto alignment = std::size_t{1} << exponent;
        Require(reinterpret_cast<std::uintptr_t>(allocation) % alignment == 0, "XMem 32-bit allocation lost its requested alignment");
        XMemFreeDefault(allocation, attributes);
    }
}
