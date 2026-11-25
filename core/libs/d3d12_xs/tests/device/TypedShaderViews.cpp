#include "Tests.hpp"
#include "CommittedBuffers.hpp"
#include "TypedBufferReference.hpp"
#include "../../src/descriptor/Descriptions.hpp"
#include <algorithm>

void AnyBox::Graphics::Tests::CheckTypedShaderViews(Device* device) {
    auto create = Method<decltype(&Device::CreateShaderResourceViewX)>(device, 70);
    D3D12_RESOURCE_DESC resource{};
    resource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource.Width = 4096;
    XboxShaderViewDescription description{};
    description.address = 0x123456780000;
    description.view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    description.view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    alignas(16) std::array<std::uint32_t, 16> words{};
    auto destination = reinterpret_cast<std::uintptr_t>(words.data() + 4);
    for (const auto& reference : TypedBufferReferences) {
        description.view.Format = static_cast<DXGI_FORMAT>(reference.format);
        for (auto first : {0ULL, 3ULL, 255ULL}) {
            description.view.Buffer.FirstElement = first;
            for (auto count : {0u, 1u, 8u, UINT32_MAX}) {
                auto available = (resource.Width - first * reference.size) / reference.size;
                if (count != UINT32_MAX && count > available) continue;
                description.view.Buffer.NumElements = count;
                words.fill(UINT32_MAX);
                create(device, &resource, &description, destination);
                auto address = description.address + first * reference.size;
                auto elements = count == UINT32_MAX ? available : count;
                Require(words[4] == static_cast<std::uint32_t>(address) && words[5] == ((address >> 32) | (reference.size << 16)) && words[6] == elements && words[7] == reference.control, "Typed buffer descriptor differs from the original encoder");
                Require(std::ranges::all_of(std::span(words).subspan(8, 4), [](auto value) { return value == 0; }), "Typed buffer upper descriptor words were not cleared");
                Require(words.front() == UINT32_MAX && words.back() == UINT32_MAX, "Typed buffer descriptor overwrote a neighboring entry");
            }
        }
    }
    description.view.Format = DXGI_FORMAT_R16_UINT;
    description.view.Buffer.FirstElement = 0;
    description.view.Buffer.NumElements = 12;
    auto buffer = MakeCommittedBuffer(device, D3D12_HEAP_TYPE_DEFAULT, resource.Width);
    auto address = Method<decltype(&Resource::GetGPUVirtualAddress)>(buffer.get(), 11)(buffer.get());
    Method<decltype(&Device::CreateShaderResourceView)>(device, 18)(device, buffer.get(), &description.view, destination);
    Require(words[4] == static_cast<std::uint32_t>(address) && words[5] == ((address >> 32) | 0x20000) && words[6] == 12 && words[7] == 0x1000B204, "Resource typed shader view encoding differs from the extended view");
    description.view.Buffer.NumElements = 2049;
    Reject<std::out_of_range>([&] { create(device, &resource, &description, destination); });
    description.view.Buffer.NumElements = 12;
    description.view.Buffer.FirstElement = UINT64_MAX;
    Reject<std::out_of_range>([&] { create(device, &resource, &description, destination); });
    description.view.Buffer.FirstElement = 0;
    resource.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    Reject<std::invalid_argument>([&] { create(device, &resource, &description, destination); });
}
