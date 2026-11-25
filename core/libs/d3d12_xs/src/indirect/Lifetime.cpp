#include "Signature.hpp"

AnyBox::Graphics::CommandSignature::CommandSignature(Device* device, RootSignature* root, const CommandSignatureDescription& description) : vtable(&GetCommandSignatureTable()), device(device), root(root), stride(description.ByteStride), nodeMask(description.NodeMask), arguments(description.pArgumentDescs, description.pArgumentDescs + description.NumArgumentDescs) {
    Device::AddRef(device);
    try { if (root) RootSignature::AddRef(root); }
    catch (...) { Device::Release(device); throw; }
}

AnyBox::Graphics::CommandSignature::~CommandSignature() {
    if (root) RootSignature::Release(root);
    Device::Release(device);
}

bool AnyBox::Graphics::CommandSignature::Supports(const Guid& interfaceId) noexcept { return interfaceId == CommandSignatureId || interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId || interfaceId == PageableId; }

AnyBox::Graphics::CommandSignature* AnyBox::Graphics::CommandSignature::From(void* object, Device* owner) {
    if (!object || reinterpret_cast<std::uintptr_t>(object) % alignof(CommandSignature) != 0) throw std::invalid_argument("Command signature is null or misaligned");
    const ObjectTable* table;
    std::memcpy(&table, object, sizeof(table));
    if (table != &GetCommandSignatureTable()) throw std::invalid_argument("Foreign command signature is unsupported");
    auto* signature = static_cast<CommandSignature*>(object);
    if (signature->device != owner) throw std::invalid_argument("Command signature belongs to another device");
    return signature;
}
