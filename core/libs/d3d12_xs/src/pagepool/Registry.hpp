#pragma once

#include "../host/Com.hpp"
#include <anybox/PhysicalMemory.hpp>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace AnyBox::Graphics {
class PagePool {
public:
    PagePool(std::uint64_t address, std::uint32_t count);
    std::uint64_t GetAddress() const noexcept { return address; }
    std::uint32_t GetPageCount() const noexcept { return count; }
    std::span<std::byte> GetPage(std::uint32_t index) const { return pages->GetPage(index); }

private:
    std::uint64_t address;
    std::uint32_t count;
    ComOwner<IPhysicalPageLease> pages;
};
class PagePools {
public:
    std::uint64_t Register(std::uint64_t address, std::uint32_t count);
    void Unregister(std::uint64_t handle);
    std::shared_ptr<const PagePool> Resolve(std::uint64_t handle) const {
        std::scoped_lock lock(mutex);
        auto found = pools.find(handle);
        if (found == pools.end()) throw std::invalid_argument("Xbox page pool handle is not registered");
        return found->second;
    }

private:
    mutable std::mutex mutex;
    std::unordered_map<std::uint64_t, std::shared_ptr<PagePool>> pools;
    std::uint64_t nextHandle = 1;
};
PagePools& GetProcessPagePools();
}
