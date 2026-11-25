#include "Tests.hpp"
#include "../../src/pagepool/Registry.hpp"
#include <xmem.hpp>

void AnyBox::Graphics::Tests::CheckPagePools(Device* device) {
    std::array<std::uint64_t, 3> frames{};
    std::size_t count = frames.size();
    Require(XMemAllocatePhysicalPages(0x280, &count, frames.data()) == 1 && count == frames.size(), "Page pool physical allocation failed");
    auto* address = static_cast<std::byte*>(XMemVirtualAlloc(nullptr, count * PhysicalPageSize, 0x20402000, 0x23, 0x2004));
    Require(address != nullptr, "Page pool virtual allocation failed");
    XMemMapPhysicalPages(address, count, frames.data());
    auto pointer = reinterpret_cast<std::uintptr_t>(address);
    auto registerPool = Method<decltype(&Device::RegisterPagePoolX)>(device, 74);
    auto unregisterPool = Method<decltype(&Device::UnregisterPagePoolX)>(device, 75);
    std::uint64_t handle = 0;
    Require(registerPool(device, pointer + 17, 3, &handle) == S_OK && handle != 0, "Page pool registration failed");
    auto retained = device->GetPagePools().Resolve(handle);
    Require(retained->GetAddress() == pointer && retained->GetPageCount() == 3, "Page pool registration did not preserve its aligned range");
    for (unsigned index = 0; index < count; ++index) {
        auto bytes = retained->GetPage(index);
        bytes[0] = static_cast<std::byte>(index + 11);
        Require(address[index * PhysicalPageSize] == bytes[0], "Page pool does not refer to the registered physical memory");
    }
    Reject<std::out_of_range>([&] { retained->GetPage(3); });
    Reject<std::logic_error>([&] { XMemMapPhysicalPages(address, count, nullptr); });
    std::uint64_t subset = 0;
    Require(registerPool(device, pointer + PhysicalPageSize, 1, &subset) == S_OK && subset != handle, "Page pool subset registration did not own a distinct handle");
    auto selected = device->GetPagePools().Resolve(subset);
    Require(selected->GetPageCount() == 1 && selected->GetPage(0)[0] == std::byte{12}, "Page pool subset selected another physical page");
    selected.reset();
    Require(unregisterPool(device, subset) == S_OK, "Page pool subset unregister failed");
    Require(unregisterPool(device, handle) == S_OK, "Page pool unregister failed");
    Reject<std::invalid_argument>([&] { device->GetPagePools().Resolve(handle); });
    Reject<std::invalid_argument>([&] { unregisterPool(device, handle); });
    Reject<std::logic_error>([&] { XMemMapPhysicalPages(address, count, nullptr); });
    retained.reset();
    handle = UINT64_MAX;
    Reject<std::invalid_argument>([&] { registerPool(device, pointer, 0, &handle); });
    Require(handle == 0, "Rejected page pool registration retained a stale handle");
    Reject<std::invalid_argument>([&] { registerPool(device, 17, 1, &handle); });
    Reject<std::invalid_argument>([&] { registerPool(device, pointer, 1, nullptr); });
    Reject<std::invalid_argument>([&] { registerPool(nullptr, pointer, 1, &handle); });
    Reject<std::runtime_error>([&] { registerPool(device, pointer, 0x80000001, &handle); });
    Reject<std::invalid_argument>([&] { registerPool(device, pointer, 4, &handle); });
    XMemMapPhysicalPages(address, count, nullptr);
    Reject<std::invalid_argument>([&] { registerPool(device, pointer, 1, &handle); });
    Require(VirtualFree(address, 0, MEM_RELEASE) != FALSE, "Page pool virtual reservation release failed");
    XMemFreePhysicalPages(count, frames.data());
}
