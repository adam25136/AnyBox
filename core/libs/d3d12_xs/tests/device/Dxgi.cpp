#include "Tests.hpp"
#include "../../src/dxgi/Device.hpp"
#include "../../src/dxgi/Adapter.hpp"
#include "DxgiReference.hpp"
#include <numeric>
#include <vector>

void AnyBox::Graphics::Tests::CheckDxgi(Device* device) {
    void* object = nullptr;
    auto query = Method<decltype(&Device::QueryInterface)>(device, 0);
    Require(query(device, DxgiDeviceId, &object) == 0 && object, "DXGI device query failed");
    auto* dxgi = static_cast<IDXGIDevice*>(object);
    void* repeated = nullptr;
    Require(query(device, DxgiObjectId, &repeated) == 0 && repeated == dxgi, "DXGI object identity changed");
    Require(static_cast<IDXGIObject*>(repeated)->Release() == 1, "DXGI object did not retain its device facade");
    GUID unknown;
    std::memcpy(&unknown, &UnknownId, sizeof(unknown));
    Require(dxgi->QueryInterface(unknown, &repeated) == S_OK && repeated == device, "DXGI device lost its controlling IUnknown");
    Method<decltype(&Device::Release)>(device, 2)(static_cast<Device*>(repeated));

    IDXGIAdapter* adapter = nullptr;
    Require(dxgi->GetAdapter(&adapter) == S_OK && adapter, "DXGI device did not return its adapter");
    DXGI_ADAPTER_DESC description{};
    Require(adapter->GetDesc(&description) == S_OK, "DXGI adapter description failed");
    auto expected = Method<decltype(&Device::GetAdapterLuid)>(device, 43)(device);
    Require(description.AdapterLuid.LowPart == expected.LowPart && description.AdapterLuid.HighPart == expected.HighPart, "DXGI adapter differs from its D3D12 device");
    auto reference = GetReferenceOutput(expected);
    IDXGIOutput* output = nullptr;
    auto result = adapter->EnumOutputs(0, &output);
    if (reference) {
        Require(result == S_OK && output, "DXGI adapter output enumeration failed");
        GUID adapterId;
        std::memcpy(&adapterId, &DxgiAdapterId, sizeof(adapterId));
        Require(output->GetParent(adapterId, &repeated) == S_OK && repeated == adapter, "DXGI output lost its parent adapter identity");
        static_cast<IDXGIAdapter*>(repeated)->Release();
    } else {
        Require(result == DXGI_ERROR_NOT_FOUND && !output, "Headless DXGI adapter reported an unexpected output");
    }
    Require(dxgi->Release() == 0, "DXGI device facade did not release its final reference");
    adapter->Release();

    if (reference) {
        DXGI_OUTPUT_DESC outputDescription{};
        Require(output->GetDesc(&outputDescription) == S_OK && outputDescription.AttachedToDesktop, "DXGI output does not describe a real connected display");
        using GetModes = HRESULT (*)(IDXGIOutput*, DXGI_FORMAT, UINT, UINT*, DXGI_MODE_DESC1*);
        auto getModes = Method<GetModes>(output, 21);
        UINT count = 0;
        Require(getModes(output, DXGI_FORMAT_UNKNOWN, 0, &count, nullptr) == S_OK && count != 0, "Xbox output slot 21 did not enumerate real display modes");
        std::vector<DXGI_MODE_DESC1> modes(count);
        Require(getModes(output, DXGI_FORMAT_UNKNOWN, 0, &count, modes.data()) == S_OK && count == modes.size(), "Xbox display mode list size changed");
        for (const auto& mode : modes) Require(mode.Width != 0 && mode.Height != 0 && mode.RefreshRate.Denominator != 0 && std::gcd(mode.RefreshRate.Numerator, mode.RefreshRate.Denominator) == 1, "Xbox display mode has invalid dimensions or refresh fraction");
        UINT capacity = 0;
        Reject<std::length_error>([&] { getModes(output, DXGI_FORMAT_UNKNOWN, 0, &capacity, modes.data()); });
        Require(capacity == count, "Rejected display enumeration did not report its required capacity");
        Reject<std::invalid_argument>([&] { getModes(output, DXGI_FORMAT_UNKNOWN, 0x80000000, &count, nullptr); });
        Reject<std::invalid_argument>([&] { getModes(output, DXGI_FORMAT_UNKNOWN, 0, nullptr, nullptr); });

        UINT referenceCount = 0;
        Require(reference->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &referenceCount, nullptr) == S_OK, "Native display mode count failed");
        std::vector<DXGI_MODE_DESC1> referenceModes(referenceCount);
        Require(referenceCount != 0 && reference->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &referenceCount, referenceModes.data()) == S_OK, "Native display mode enumeration failed");
        Require(getModes(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &count, nullptr) == S_OK && count == referenceCount, "Xbox display mode count differs from native enumeration");
        modes.resize(count);
        Require(getModes(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &count, modes.data()) == S_OK, "Xbox display format enumeration failed");
        for (UINT index = 0; index < count; ++index) {
            auto& mode = referenceModes[index];
            auto divisor = std::gcd(mode.RefreshRate.Numerator, mode.RefreshRate.Denominator);
            mode.RefreshRate.Numerator /= divisor;
            mode.RefreshRate.Denominator /= divisor;
            Require(std::memcmp(&mode, &modes[index], sizeof(mode)) == 0, "Xbox display mode differs from its real native mode");
        }
        Require(output->Release() == 0, "DXGI output final reference was not released");
    }

    Require(query(device, DxgiDeviceId, &object) == 0 && object, "DXGI device facade could not be recreated");
    dxgi = static_cast<IDXGIDevice*>(object);
    Reject<std::runtime_error>([&] { dxgi->SetGPUThreadPriority(0); });
    Require(dxgi->Release() == 0, "Recreated DXGI device facade leaked its reference");
}
