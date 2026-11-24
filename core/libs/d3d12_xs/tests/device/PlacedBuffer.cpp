#include "Buffers.hpp"
#include "../../src/resource/Resource.hpp"
#include "PlacedTransfer.hpp"
#include "ResourceMetadata.hpp"
#include "BufferViews.hpp"
#include <xmem.hpp>

void AnyBox::Graphics::Tests::CheckPlacedBuffer(Device* device) {
    constexpr auto page = PhysicalPageSize;
    std::array<std::uint64_t, 2> frames{};
    auto count = frames.size();
    Require(XMemAllocatePhysicalPages(0x280, &count, frames.data()) == 1, "Placed buffer physical allocation failed");
    auto* address = static_cast<std::byte*>(XMemVirtualAlloc(nullptr, 2 * page, 0x20402000, 0x23, 0x2004));
    XMemMapPhysicalPages(address, 2, frames.data());
    D3D12_RESOURCE_DESC1 description{};
    description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    description.Width = 2 * page;
    description.Height = 1;
    description.DepthOrArraySize = 1;
    description.MipLevels = 1;
    description.SampleDesc.Count = 1;
    description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    description.Flags = static_cast<D3D12_RESOURCE_FLAGS>(0x20004);
    auto create = Method<decltype(&Device::CreatePlacedResourceLayoutX)>(device, 133);
    auto pointer = reinterpret_cast<std::uintptr_t>(address);
    void* output = nullptr;
    Require(create(device, pointer, &description, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, ResourceId, &output) == S_OK && output, "Xbox placed buffer creation failed");
    auto* resource = static_cast<Resource*>(output);
    auto release = Method<decltype(&Resource::Release)>(resource, 2);
    auto query = Method<decltype(&Resource::QueryInterface)>(resource, 0);
    for (const auto& id : {ResourceId, Resource1Id, Resource2Id, UnknownId, ObjectId, DeviceChildId, PageableId}) {
        void* identity = nullptr;
        Require(query(resource, id, &identity) == 0 && identity == resource, "Placed buffer COM identity changed");
        Require(release(static_cast<Resource*>(identity)) == 1, "Placed buffer interface did not retain its reference");
    }
    auto base = Method<decltype(&Resource::GetDescription)>(resource, 10)(resource);
    auto extended = Method<decltype(&Resource::GetDescription1)>(resource, 16)(resource);
    Require(base.Width == description.Width && base.Flags == description.Flags && extended.Width == description.Width && extended.Flags == description.Flags, "Xbox buffer description lost its original metadata");
    Require(Method<decltype(&Resource::GetGPUVirtualAddress)>(resource, 11)(resource) == pointer, "Xbox GPU buffer did not retain its title address");
    void* mapped = nullptr;
    auto map = Method<decltype(&Resource::Map)>(resource, 8);
    auto unmap = Method<decltype(&Resource::Unmap)>(resource, 9);
    Require(map(resource, 0, nullptr, &mapped) == S_OK && mapped == address, "Xbox buffer CPU map returned a different title address");
    static_cast<std::byte*>(mapped)[17] = std::byte{0x73};
    unmap(resource, 0, nullptr);
    Require(address[17] == std::byte{0x73}, "Xbox buffer mapping is not coherent with title memory");
    CheckPlacedBufferTransfer(device, resource, {address, 2 * page}, false);
    CheckResourceMetadata(device, resource);
    CheckBufferViews(device, resource);
    auto properties = Method<decltype(&Resource::GetHeapProperties)>(resource, 14);
    D3D12_HEAP_PROPERTIES heap{};
    Reject<std::invalid_argument>([&] { properties(resource, nullptr, nullptr); });
    Reject<std::runtime_error>([&] { properties(resource, &heap, nullptr); });
    Reject<std::logic_error>([&] { XMemMapPhysicalPages(address, 2, nullptr); });
    Reject<std::invalid_argument>([&] { create(device, pointer, nullptr, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, ResourceId, &output); });
    Require(!output, "Rejected placed resource creation retained its output");
    Reject<std::invalid_argument>([&] { create(device, pointer + 1, &description, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, ResourceId, &output); });
    Reject<std::invalid_argument>([&] { create(device, pointer, &description, D3D12_BARRIER_LAYOUT_COMMON, nullptr, ResourceId, &output); });
    Reject<std::invalid_argument>([&] { map(resource, 1, nullptr, &mapped); });
    D3D12_RANGE invalid{0, 2 * page + 1};
    Reject<std::invalid_argument>([&] { unmap(resource, 0, &invalid); });
    CheckPlacedBufferLifetime(device, resource, {address, 2 * page});
    for (bool repeated : {false, true}) {
        std::array mapping{repeated ? frames[0] : frames[1], frames[0]};
        XMemMapPhysicalPages(address, mapping.size(), mapping.data());
        Require(create(device, pointer, &description, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, ResourceId, &output) == S_OK && output, "Xbox scatter buffer creation failed");
        auto* scatter = static_cast<Resource*>(output);
        CheckPlacedBufferTransfer(device, scatter, {address, 2 * page}, repeated);
        CheckBufferViews(device, scatter);
        Reject<std::logic_error>([&] { XMemMapPhysicalPages(address, 2, nullptr); });
        Require(release(scatter) == 0, "Xbox scatter buffer final reference was not released");
    }
    XMemMapPhysicalPages(address, 2, nullptr);
    for (std::size_t index = 0; index < frames.size(); ++index) Require(VirtualFree(address + index * page, 0, MEM_RELEASE) != FALSE, "Placed buffer test placeholder did not release");
    XMemFreePhysicalPages(frames.size(), frames.data());
}
