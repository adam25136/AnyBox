#include "Device.hpp"
#include "../host/Dxgi.hpp"
#include "../host/Library.hpp"
#include "../memory/Configuration.hpp"
#include <d3d12_xs.hpp>
#include <anybox/Unimplemented.hpp>
#include <anybox/vulkan/Context.hpp>
#include <stdexcept>
#include <string>

namespace {
std::int32_t CreateGraphicsDevice(PFN_D3D12_CREATE_DEVICE create, LUID defaultLuid, const D3D12XboxCreateDeviceParameters& parameters, void** device) {
    using namespace AnyBox::Graphics;
    auto factory = CreateHostDxgiFactory();
    std::string incompatibleReason;
    for (UINT index = 0;; ++index) {
        IDXGIAdapter1* output = nullptr;
        auto result = factory->EnumAdapters1(index, &output);
        NativeAdapter candidate(output);
        if (result == DXGI_ERROR_NOT_FOUND) break;
        CheckGraphicsResult(result, "IDXGIFactory4::EnumAdapters1");
        if (!candidate) throw std::runtime_error("DXGI adapter enumeration returned no interface");
        DXGI_ADAPTER_DESC1 description{};
        CheckGraphicsResult(candidate->GetDesc1(&description), "IDXGIAdapter1::GetDesc1");
        if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) continue;
        if (description.AdapterLuid.LowPart == defaultLuid.LowPart && description.AdapterLuid.HighPart == defaultLuid.HighPart) continue;

        void* host = nullptr;
        result = create(candidate.get(), D3D_FEATURE_LEVEL_12_0, NativeGuid(BaseDeviceId), &host);
        ComOwner<ID3D12Device> ownedHost(static_cast<ID3D12Device*>(host));
        if (result == DXGI_ERROR_UNSUPPORTED) continue;
        CheckGraphicsResult(result, "D3D12CreateDevice for DXGI adapter");
        if (!ownedHost) throw std::runtime_error("D3D12CreateDevice returned no interface");
        try {
            *device = AcquireDevice(ownedHost.get(), parameters);
            return static_cast<std::int32_t>(result);
        } catch (const AnyBox::Vulkan::AdapterNotFound&) {
        } catch (const AnyBox::Vulkan::AdapterIncompatible& error) {
            incompatibleReason = error.what();
        }
    }
    if (!incompatibleReason.empty()) throw AnyBox::Vulkan::AdapterIncompatible("No compatible DXGI and Vulkan adapter: " + incompatibleReason);
    throw AnyBox::Vulkan::AdapterNotFound("No DXGI hardware adapter has a matching Vulkan device");
}
}

std::int32_t D3D12XboxCreateDevice(IGraphicsUnknown* adapter, const D3D12XboxCreateDeviceParameters* parameters, const AnyBox::Guid& interfaceId, void** device) noexcept(false) {
    if (!device) throw std::invalid_argument("Graphics device output is null");
    *device = nullptr;
    if (!parameters) throw std::invalid_argument("Xbox graphics device parameters are null");
    if (parameters->version != 0x20004 || parameters->processDebugFlags != 0) ANYBOX_NOT_IMPLEMENTED();
    if (!AnyBox::Graphics::Device::Supports(interfaceId)) throw std::invalid_argument("Unsupported Xbox graphics device interface");
    if (adapter) ANYBOX_NOT_IMPLEMENTED();
    static auto create = AnyBox::Graphics::GetHostLibrary().Get<PFN_D3D12_CREATE_DEVICE>("D3D12CreateDevice");
    void* host = nullptr;
    auto result = create(nullptr, D3D_FEATURE_LEVEL_12_0, AnyBox::Graphics::NativeGuid(AnyBox::Graphics::BaseDeviceId), &host);
    AnyBox::Graphics::ComOwner<ID3D12Device> ownedHost(static_cast<ID3D12Device*>(host));
    AnyBox::Graphics::CheckGraphicsResult(result, "D3D12CreateDevice");
    if (!ownedHost) throw std::runtime_error("D3D12CreateDevice returned no interface");
    AnyBox::Graphics::EnsureGraphicsMemoryConfigured();
    try {
        *device = AnyBox::Graphics::AcquireDevice(ownedHost.get(), *parameters);
    } catch (const AnyBox::Vulkan::AdapterNotFound&) {
        return CreateGraphicsDevice(create, ownedHost->GetAdapterLuid(), *parameters, device);
    }
    return static_cast<std::int32_t>(result);
}
