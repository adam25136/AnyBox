#pragma once

#include "Tests.hpp"
#include "../../src/indirect/Signature.hpp"
#include <memory>

namespace AnyBox::Graphics::Tests {
struct IndirectDeleter {
    void operator()(CommandSignature* signature) const { Method<decltype(&CommandSignature::Release)>(signature, 2)(signature); }
};
using OwnedCommandSignature = std::unique_ptr<CommandSignature, IndirectDeleter>;
inline OwnedCommandSignature MakeCommandSignature(Device* device, const CommandSignatureDescription& description, RootSignature* root = nullptr) {
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateCommandSignature)>(device, 41)(device, &description, root, CommandSignatureId, &output) == S_OK && output, "Command signature creation failed");
    return OwnedCommandSignature(static_cast<CommandSignature*>(output));
}
void CheckIndirectRoots(Device* device);
void CheckIndirectInvalid(Device* device);
}
