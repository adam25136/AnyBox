#include "Dxgi.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <stdexcept>

namespace {
class DxgiLibrary {
public:
    DxgiLibrary() : module(LoadLibraryExW(L"dxgi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)) {
        if (!module) AnyBox::Windows::ThrowLastError("LoadLibraryExW DXGI");
        try {
            auto address = GetProcAddress(module, "CreateDXGIFactory2");
            if (!address) AnyBox::Windows::ThrowLastError("GetProcAddress CreateDXGIFactory2");
            static_assert(sizeof(create) == sizeof(address));
            std::memcpy(&create, &address, sizeof(create));
            HMODULE pinned = nullptr;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<const wchar_t*>(module), &pinned)) AnyBox::Windows::ThrowLastError("GetModuleHandleExW DXGI lifetime");
        } catch (...) {
            FreeLibrary(module);
            throw;
        }
    }
    ~DxgiLibrary() { FreeLibrary(module); }
    AnyBox::Graphics::ComOwner<IDXGIFactory4> CreateFactory() const {
        constexpr GUID interfaceId{0x1BC6EA02, 0xEF36, 0x464F, {0xBF, 0x0C, 0x21, 0xCA, 0x39, 0xE5, 0x16, 0x8A}};
        void* output = nullptr;
        auto result = create(0, interfaceId, &output);
        AnyBox::Graphics::ComOwner<IDXGIFactory4> factory(static_cast<IDXGIFactory4*>(output));
        AnyBox::Windows::CheckHResult(result, "CreateDXGIFactory2");
        if (!factory) throw std::runtime_error("DXGI factory interface is null");
        return factory;
    }
    DxgiLibrary(const DxgiLibrary&) = delete;
    DxgiLibrary& operator=(const DxgiLibrary&) = delete;

private:
    HMODULE module;
    HRESULT (WINAPI *create)(UINT, REFIID, void**) = nullptr;
};
}

AnyBox::Graphics::ComOwner<IDXGIFactory4> AnyBox::Graphics::CreateHostDxgiFactory() {
    static DxgiLibrary library;
    return library.CreateFactory();
}

AnyBox::Graphics::NativeAdapter AnyBox::Graphics::GetHostAdapter(Device* device) {
    static const auto factory = CreateHostDxgiFactory();
    constexpr GUID interfaceId{0x29038F61, 0x3839, 0x4626, {0x91, 0xFD, 0x08, 0x68, 0x79, 0x01, 0x1A, 0x05}};
    void* output = nullptr;
    Windows::CheckHResult(factory->EnumAdapterByLuid(device->GetHost()->GetAdapterLuid(), interfaceId, &output), "DXGI adapter lookup by device LUID");
    if (!output) throw std::runtime_error("DXGI adapter interface is null");
    return NativeAdapter(static_cast<IDXGIAdapter1*>(output));
}
