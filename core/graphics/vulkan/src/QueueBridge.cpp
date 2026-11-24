#include <anybox/vulkan/QueueBridge.hpp>
#include <anybox/platform/WindowsError.hpp>
#include <d3d12.h>
#include <cstring>

AnyBox::Vulkan::QueueBridge::QueueBridge(Context& context, ID3D12CommandQueue& host, std::uint32_t queueIndex) : context(context), host(host), queueIndex(queueIndex) {
    (void)context.GetQueue(queueIndex);
    constexpr GUID deviceId{0x189819F1, 0x1DB6, 0x4B57, {0xBE, 0x54, 0x18, 0x21, 0x33, 0x9B, 0x85, 0xF7}};
    constexpr GUID fenceId{0x0A753DCF, 0xC4D8, 0x4B91, {0xAD, 0xF6, 0xBE, 0x5A, 0x60, 0xD9, 0x5A, 0x76}};
    void* native = nullptr;
    Windows::CheckHResult(host.GetDevice(deviceId, &native), "Query queue bridge D3D12 device");
    if (!native) throw std::runtime_error("Queue bridge native device is null");
    auto release = [](ID3D12Device* device) { device->Release(); };
    std::unique_ptr<ID3D12Device, decltype(release)> device(static_cast<ID3D12Device*>(native), release);
    auto luid = device->GetAdapterLuid();
    if (std::memcmp(&luid, context.GetAdapterLuid().data(), sizeof(luid)) != 0) throw std::invalid_argument("Queue bridge D3D12 and Vulkan adapters differ");
    native = nullptr;
    Windows::CheckHResult(device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, fenceId, &native), "Create queue bridge shared fence");
    fence = static_cast<ID3D12Fence*>(native);
    if (!fence) throw std::runtime_error("Queue bridge shared fence is null");
    try {
        HANDLE shared = nullptr;
        Windows::CheckHResult(device->CreateSharedHandle(fence, nullptr, GENERIC_ALL, nullptr, &shared), "Export queue bridge fence");
        auto close = [](void* handle) { CloseHandle(handle); };
        std::unique_ptr<void, decltype(close)> handle(shared, close);
        timeline = std::make_unique<Timeline>(Timeline::ImportD3D12Fence(context, shared));
        host.AddRef();
    } catch (...) {
        fence->Release();
        throw;
    }
}

void AnyBox::Vulkan::QueueBridge::Check() const {
    if (failed) throw std::runtime_error("Queue bridge has an incomplete failed submission");
}
