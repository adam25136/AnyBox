#include "Device.hpp"
#include "../indirect/Signature.hpp"
#include <memory>

HRESULT AnyBox::Graphics::Device::CreateCommandSignature(Device* self, const CommandSignatureDescription* description, void* rootSignature, const Guid& interfaceId, void** signature) {
    if (signature) *signature = nullptr;
    if (!self || !description) throw std::invalid_argument("Command signature device or description is null");
    if (!CommandSignature::Supports(interfaceId)) throw std::invalid_argument("Unsupported command signature creation interface");
    auto* root = rootSignature ? RootSignature::From(rootSignature, self) : nullptr;
    ValidateIndirectLayout(*description, root);
    if (!signature) return S_FALSE;
    auto object = std::make_unique<CommandSignature>(self, root, *description);
    *signature = object.release();
    return S_OK;
}
