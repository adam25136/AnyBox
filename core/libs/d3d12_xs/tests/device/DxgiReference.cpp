#include "DxgiReference.hpp"
#include "Tests.hpp"

AnyBox::Graphics::ComOwner<IDXGIOutput1> AnyBox::Graphics::Tests::GetReferenceOutput(LUID adapter) {
    auto module = GetModuleHandleW(L"dxgi.dll");
    if (!module) Windows::ThrowLastError("GetModuleHandleW DXGI reference");
    auto address = GetProcAddress(module, "CreateDXGIFactory2");
    if (!address) Windows::ThrowLastError("GetProcAddress DXGI reference factory");
    using Create = HRESULT (WINAPI *)(UINT, REFIID, void**);
    Create create;
    std::memcpy(&create, &address, sizeof(create));
    constexpr GUID factoryId{0x1BC6EA02, 0xEF36, 0x464F, {0xBF, 0x0C, 0x21, 0xCA, 0x39, 0xE5, 0x16, 0x8A}};
    constexpr GUID adapterId{0x2411E7E1, 0x12AC, 0x4CCF, {0xBD, 0x14, 0x97, 0x98, 0xE8, 0x53, 0x4D, 0xC0}};
    constexpr GUID outputId{0x00CDDEA8, 0x939B, 0x4B83, {0xA3, 0x40, 0xA6, 0x85, 0x22, 0x66, 0x66, 0xCC}};
    void* object = nullptr;
    Require(create(0, factoryId, &object) == S_OK && object, "Reference DXGI factory creation failed");
    ComOwner<IDXGIFactory4> factory(static_cast<IDXGIFactory4*>(object));
    Require(factory->EnumAdapterByLuid(adapter, adapterId, &object) == S_OK && object, "Reference DXGI adapter lookup failed");
    ComOwner<IDXGIAdapter> nativeAdapter(static_cast<IDXGIAdapter*>(object));
    IDXGIOutput* output = nullptr;
    auto result = nativeAdapter->EnumOutputs(0, &output);
    if (result == DXGI_ERROR_NOT_FOUND) return {};
    Require(result == S_OK && output, "Reference DXGI output lookup failed");
    ComOwner<IDXGIOutput> nativeOutput(output);
    Require(nativeOutput->QueryInterface(outputId, &object) == S_OK && object, "Reference DXGI output interface query failed");
    return ComOwner<IDXGIOutput1>(static_cast<IDXGIOutput1*>(object));
}
