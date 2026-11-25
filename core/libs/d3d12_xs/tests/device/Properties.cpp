#include "Tests.hpp"
#include "../../src/host/Com.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <anybox/vulkan/Context.hpp>

void AnyBox::Graphics::Tests::CheckProperties(Device* device) {
    constexpr GUID HostId{0x189819F1, 0x1DB6, 0x4B57, {0xBE, 0x54, 0x18, 0x21, 0x33, 0x9B, 0x85, 0xF7}};
    auto module = GetModuleHandleW(L"d3d12.dll");
    if (!module) Windows::ThrowLastError("GetModuleHandleW d3d12");
    auto address = GetProcAddress(module, "D3D12CreateDevice");
    if (!address) Windows::ThrowLastError("GetProcAddress D3D12CreateDevice");
    PFN_D3D12_CREATE_DEVICE create;
    std::memcpy(&create, &address, sizeof(create));
    void* native = nullptr;
    auto result = create(nullptr, D3D_FEATURE_LEVEL_12_0, HostId, &native);
    ComOwner<ID3D12Device> defaultDevice(static_cast<ID3D12Device*>(native));
    Require(result == S_OK && defaultDevice, "Reference D3D12 device creation failed");

    auto* host = device->GetHost();
    auto luid = Method<decltype(&Device::GetAdapterLuid)>(device, 43)(device);
    auto expectedLuid = host->GetAdapterLuid();
    Require(luid.LowPart == expectedLuid.LowPart && luid.HighPart == expectedLuid.HighPart, "Adapter identity changed across the Xbox ABI");
    auto defaultLuid = defaultDevice->GetAdapterLuid();
    if (defaultLuid.LowPart != luid.LowPart || defaultLuid.HighPart != luid.HighPart) {
        Vulkan::AdapterLuid bytes{};
        static_assert(sizeof(defaultLuid) == sizeof(bytes));
        std::memcpy(bytes.data(), &defaultLuid, bytes.size());
        Reject<Vulkan::AdapterNotFound>([&] { Vulkan::Context context(bytes, 5); });
    }
    Require(Method<decltype(&Device::GetDeviceRemovedReason)>(device, 37)(device) == host->GetDeviceRemovedReason(), "Graphics device removal status differs from the host");
    auto count = Method<decltype(&Device::GetNodeCount)>(device, 7);
    Require(count(device) == host->GetNodeCount() && count(device) > 0, "Xbox graphics node count differs from its host");
    auto increment = Method<decltype(&Device::GetDescriptorHandleIncrementSize)>(device, 15);
    constexpr std::array<std::uint32_t, 5> descriptorSizes{32, 16, 112, 128, 16};
    for (std::uint32_t index = 0; index < descriptorSizes.size(); ++index) {
        Require(increment(device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(index)) == descriptorSizes[index], "Xbox descriptor increment differs from the console ABI");
    }
    Reject<std::invalid_argument>([&] { increment(device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(5)); });
    Reject<std::invalid_argument>([&] { increment(device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(-1)); });
    auto heap = Method<decltype(&Device::GetCustomHeapProperties)>(device, 26);
    for (auto type : {D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_TYPE_READBACK}) {
        auto actual = heap(device, 1, type);
        auto expected = host->GetCustomHeapProperties(1, type);
        Require(actual.Type == expected.Type && actual.CPUPageProperty == expected.CPUPageProperty && actual.MemoryPoolPreference == expected.MemoryPoolPreference, "Xbox graphics heap properties differ from the host");
        Require(actual.CreationNodeMask == expected.CreationNodeMask && actual.VisibleNodeMask == expected.VisibleNodeMask, "Xbox graphics heap node masks differ from the host");
    }
    if (count(device) == 1) {
        auto actual = heap(device, 0, D3D12_HEAP_TYPE_UPLOAD);
        auto expected = host->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_UPLOAD);
        Require(actual.Type == expected.Type && actual.CreationNodeMask == expected.CreationNodeMask && actual.VisibleNodeMask == expected.VisibleNodeMask, "Single-node heap mask zero changed its host semantics");
    }
    Reject<std::invalid_argument>([&] { heap(device, 1, static_cast<D3D12_HEAP_TYPE>(0)); });
    Reject<std::invalid_argument>([&] { heap(device, 1, D3D12_HEAP_TYPE_CUSTOM); });
    Reject<std::invalid_argument>([&] { heap(device, 3, D3D12_HEAP_TYPE_DEFAULT); });
    auto setName = Method<decltype(&Device::SetName)>(device, 6);
    Require(setName(device, L"AnyBox graphics ABI test") == 0, "Graphics device naming failed");
    Reject<std::invalid_argument>([&] { setName(device, nullptr); });
    constexpr Guid key{0x48AB62E7, 0xBE61, 0x44A8, {0xAB, 0x52, 0x07, 0x59, 0x31, 0x18, 0xB3, 0x62}};
    auto set = Method<decltype(&Device::SetPrivateData)>(device, 4);
    auto get = Method<decltype(&Device::GetPrivateData)>(device, 3);
    std::uint32_t value = 0x73591234;
    Require(set(device, key, sizeof(value), &value) == 0, "Graphics device private data storage failed");
    std::uint32_t output = 0, size = sizeof(output);
    Require(get(device, key, &size, &output) == 0 && size == sizeof(output) && output == value, "Graphics device private data round trip failed");
    Reject<std::invalid_argument>([&] { get(device, key, nullptr, &output); });
    Reject<std::invalid_argument>([&] { set(device, key, 4, nullptr); });
}
