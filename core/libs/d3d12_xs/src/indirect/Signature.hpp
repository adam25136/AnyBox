#pragma once

#include "Description.hpp"
#include "../device/Device.hpp"
#include "../object/Lifetime.hpp"
#include "../object/ManagedProperties.hpp"
#include "../root/Signature.hpp"
#include <vector>

namespace AnyBox::Graphics {
inline constexpr Guid CommandSignatureId{0xC36A797C, 0xEC80, 0x4F0A, {0x89, 0x85, 0xA7, 0xB2, 0x47, 0x50, 0x82, 0xD1}};
const ObjectTable& GetCommandSignatureTable();
void ValidateIndirectLayout(const CommandSignatureDescription& description, const RootSignature* root);

class CommandSignature : public ObjectLifetime<CommandSignature, ManagedProperties> {
public:
    CommandSignature(Device* device, RootSignature* root, const CommandSignatureDescription& description);
    ~CommandSignature();
    static bool Supports(const Guid& interfaceId) noexcept;
    static CommandSignature* From(void* object, Device* owner);
    static constexpr std::size_t GetVtableOffset() { return offsetof(CommandSignature, vtable); }
    CommandSignatureDescription GetDescription() const noexcept { return {stride, static_cast<UINT>(arguments.size()), arguments.data(), nodeMask}; }
    RootSignature* GetRoot() const noexcept { return root; }
    PrivateData& GetPrivateDataStore() { return privateData.Get(); }

private:
    friend class ObjectLifetime<CommandSignature, AnyBox::Graphics::ManagedProperties>;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    const ObjectTable* vtable;
    std::atomic<std::uint32_t> references{1};
    Device* device;
    RootSignature* root;
    std::uint32_t stride;
    std::uint32_t nodeMask;
    std::vector<IndirectArgumentDescription> arguments;
    PrivateDataSlot privateData;
};
static_assert(std::is_standard_layout_v<CommandSignature>);
static_assert(CommandSignature::GetVtableOffset() == 0);
}
