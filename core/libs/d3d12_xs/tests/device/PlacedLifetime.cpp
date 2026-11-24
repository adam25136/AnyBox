#include "Tests.hpp"
#include "PlacedTransfer.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <xmem.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckPlacedBufferLifetime(Device* device, Resource* resource, std::span<std::byte> memory) {
    auto guest = reinterpret_cast<std::uintptr_t>(memory.data());
    auto& addresses = device->GetBufferAddresses();
    auto& buffer = resource->GetGpuBuffer();
    auto mapping = addresses.Resolve(guest, memory.size());
    Require(mapping->Translate(guest + 17, 4) == buffer.GetAddress() + 17, "Placed buffer address translation lost its byte offset");
    Reject<std::out_of_range>([&] { (void)addresses.Resolve(guest + memory.size() - 1, 2); });
    auto description = resource->GetMetadata();
    description.Width = PhysicalPageSize;
    void* output = nullptr;
    auto create = Method<decltype(&Device::CreatePlacedResourceLayoutX)>(device, 133);
    Require(create(device, guest, &description, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, ResourceId, &output) == S_OK, "Aliased buffer creation failed");
    auto* alias = static_cast<Resource*>(output);
    Require(addresses.Resolve(guest).get() == mapping.get(), "A smaller aliased buffer replaced the containing address range");
    auto snapshot = addresses.Capture();
    Require(snapshot.mappings.size() == 2, "Buffer snapshot did not retain both aliased resources");
    auto release = Method<decltype(&Resource::Release)>(resource, 2);
    Require(release(alias) == 0 && release(resource) == 0, "Placed buffer final references were not released");
    mapping.reset();
    Reject<std::logic_error>([&] { XMemMapPhysicalPages(memory.data(), memory.size() / PhysicalPageSize, nullptr); });

    constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), memory.size(), usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    std::vector<std::byte> expected(memory.begin(), memory.end());
    Vulkan::Tests::Copy(device->GetGpuContext(), buffer, scratch, memory.size());
    Require(std::ranges::equal(expected, scratch.Map()), "A retained buffer mapping lost its Vulkan memory after resource release");
    Require(addresses.Resolve(guest, memory.size())->Translate(guest) == buffer.GetAddress(), "Released resource mapping did not retain its device address");
    snapshot.mappings.clear();
    Require(addresses.Capture().mappings.empty(), "Released buffer mappings retained a resource cycle");
    Reject<std::out_of_range>([&] { (void)addresses.Resolve(guest); });
}
