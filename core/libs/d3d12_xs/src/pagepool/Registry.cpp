#include "Registry.hpp"
#include <limits>
#include <utility>

AnyBox::Graphics::PagePool::PagePool(std::uint64_t address, std::uint32_t count) : address(address), count(count), pages(AnyBoxAcquireMappedPhysicalPages(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(address)), count)) {}

std::uint64_t AnyBox::Graphics::PagePools::Register(std::uint64_t address, std::uint32_t count) {
    address &= ~std::uint64_t{65535};
    if (address == 0 || count == 0) throw std::invalid_argument("Xbox page pool address or page count is zero");
    if ((count & 0x80000000) != 0) throw std::runtime_error("Xbox large page pool mode is not implemented");
    auto pool = std::make_shared<PagePool>(address, count);
    std::scoped_lock lock(mutex);
    if (nextHandle == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Xbox page pool handle space is exhausted");
    auto handle = nextHandle;
    if (!pools.emplace(handle, std::move(pool)).second) throw std::logic_error("Xbox page pool handle was already registered");
    ++nextHandle;
    return handle;
}

void AnyBox::Graphics::PagePools::Unregister(std::uint64_t handle) {
    std::shared_ptr<PagePool> released;
    {
        std::scoped_lock lock(mutex);
        auto found = pools.find(handle);
        if (found == pools.end()) throw std::invalid_argument("Xbox page pool handle is not registered");
        released = std::move(found->second);
        pools.erase(found);
    }
}

AnyBox::Graphics::PagePools& AnyBox::Graphics::GetProcessPagePools() {
    static PagePools pools;
    return pools;
}
