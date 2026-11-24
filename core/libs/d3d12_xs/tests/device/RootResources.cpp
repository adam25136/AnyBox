#include "Tests.hpp"
#include "../../src/resource/Resource.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/shader/runtime/RootResources.hpp>

void AnyBox::Graphics::Tests::CheckShaderBufferArguments(Device* device, Resource* resource) {
    using namespace Shader;
    auto base = Method<decltype(&Resource::GetGPUVirtualAddress)>(resource, 11)(resource);
    auto width = resource->GetMetadata().Width;
    D3D12_DESCRIPTOR_HEAP_DESC description{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &description, DescriptorHeapId, &output) == S_OK, "Root argument descriptor heap creation failed");
    auto* heap = static_cast<DescriptorHeap*>(output);
    auto start = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap);
    Require(start <= UINT32_MAX, "Shader-visible table exceeds the Xbox root address encoding");
    auto table = static_cast<std::uint32_t>(start);
    auto cbv = Method<decltype(&Device::CreateConstantBufferView)>(device, 17);
    cbv(device, nullptr, start + 64);
    std::uint64_t nativeNull;
    std::memcpy(&nativeNull, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(start + 64)), sizeof(nativeNull));
    nativeNull -= 0x10000000000000ULL;
    Runtime::RootResources resources(device->GetBufferAddresses(), device->GetDescriptorAddresses(), nativeNull);
    auto check = [&](RootBufferValue value, std::uint64_t address, std::uint64_t bytes) { Require(value.address == address && value.byteLength == bytes, "Resolved shader buffer argument has an incorrect guest address or extent"); };
    BufferBinding raw{ResourceClass::ShaderResource, 0, 0, 3, 1, BufferKind::Raw};
    BufferBinding unordered{ResourceClass::UnorderedAccess, 0, 0, 4, 1, BufferKind::Raw};
    BufferBinding constant{ResourceClass::ConstantBuffer, 0, 0, 5, 1, BufferKind::Constant, 0, 64};
    BufferBinding structured{ResourceClass::ShaderResource, 0, 0, 6, 1, BufferKind::Structured, 16};
    check(resources.ResolveDescriptor(raw, base + 64), base + 64, width - 64);
    check(resources.ResolveDescriptor(structured, base + 32), base + 32, width - 32);
    check(resources.ResolveDescriptor(constant, base + 256), base + 256, 64);
    check(resources.ResolveDescriptor(unordered, 0), 0, 0);
    check(resources.ResolveDescriptor(constant, base + width - 16), base + width - 16, 16);
    Reject<std::out_of_range>([&] { resources.ResolveDescriptor(raw, base + width); });
    auto invalid = raw;
    invalid.hasCounter = true;
    Reject<std::runtime_error>([&] { resources.ResolveDescriptor(invalid, base); });
    invalid = raw;
    invalid.stride = 16;
    Reject<std::invalid_argument>([&] { resources.ResolveDescriptor(invalid, base); });

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Format = DXGI_FORMAT_R32_TYPELESS;
    srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Buffer.FirstElement = 4;
    srv.Buffer.NumElements = 8;
    srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    auto shaderView = Method<decltype(&Device::CreateShaderResourceView)>(device, 18);
    shaderView(device, resource, &srv, start);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
    uav.Format = DXGI_FORMAT_R32_TYPELESS;
    uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav.Buffer.FirstElement = 2;
    uav.Buffer.NumElements = 16;
    uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    Method<decltype(&Device::CreateUnorderedAccessView)>(device, 19)(device, resource, nullptr, &uav, start + 32);
    shaderView(device, nullptr, nullptr, start + 96);
    check(resources.ResolveTable(raw, table, 0), base + 16, 32);
    check(resources.ResolveTable(unordered, table, 1), base + 8, 64);
    check(resources.ResolveTable(raw, table, 3), 0, 0);
    check(resources.ResolveTable(constant, table, 2), 0, 0);
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDescription{base + 256, 256};
    cbv(device, &cbvDescription, start + 64);
    check(resources.ResolveTable(constant, table, 2), base + 256, 256);
    Require(resources.GetMappings().size() == 2, "Root argument resolution did not deduplicate its retained mappings");
    Reject<std::runtime_error>([&] { resources.ResolveTable(raw, table, 1); });
    Reject<std::runtime_error>([&] { resources.ResolveTable(structured, table, 0); });
    Reject<std::out_of_range>([&] { resources.ResolveTable(raw, table, 4); });
    Reject<std::out_of_range>([&] { resources.ResolveTable(raw, table, UINT32_MAX); });
    Reject<std::invalid_argument>([&] { resources.ResolveTable(raw, table + 1, 0); });

    Container::RootSignatureLayout root{};
    root.version = 2;
    Container::RootParameter parameter{};
    parameter.kind = Container::RootParameterKind::Table;
    parameter.ranges = {{Container::RootRangeKind::ShaderResource, 1, 3, 0, 0, 0}};
    root.parameters.push_back(parameter);
    Commands::RootState state(root);
    state.Write(0, 0, std::span(&table, 1));
    RootArgumentPlan plan(std::span(&raw, 1), root);
    std::array registers{0u, 1u, 2u, 3u};
    RootArgumentWriter writer(plan, registers);
    std::array<std::uint32_t, 4> packed;
    writer.Write(state, resources, packed);
    Require(packed == std::array{static_cast<std::uint32_t>(base + 16), static_cast<std::uint32_t>((base + 16) >> 32), 32u, 0u}, "Root argument writer did not preserve the application address ABI");
    Require(Method<decltype(&DescriptorHeap::Release)>(heap, 2)(heap) == 0, "Root argument descriptor heap retained its COM reference");
    check(resources.ResolveTable(raw, table, 0), base + 16, 32);
}
