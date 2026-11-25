#include "Indirect.hpp"

void AnyBox::Graphics::Tests::CheckCommandSignatures(Device* device) {
    auto create = Method<decltype(&Device::CreateCommandSignature)>(device, 41);
    for (auto type : std::array<std::uint32_t, 5>{D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH, XboxDispatchArgument}) {
        auto size = type == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW ? 16u : type == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED ? 20u : 12u;
        IndirectArgumentDescription argument{};
        argument.Type = type;
        CommandSignatureDescription description{size, 1, &argument, 1};
        Require(create(device, &description, nullptr, CommandSignatureId, nullptr) == S_FALSE, "Command signature validation mode changed its result");
        for (auto padding : {0u, 4u, 256u}) {
            description.ByteStride = size + padding;
            auto signature = MakeCommandSignature(device, description);
            auto stored = signature->GetDescription();
            Require(stored.ByteStride == size + padding && stored.NumArgumentDescs == 1 && stored.NodeMask == 1 && stored.pArgumentDescs[0].Type == type, "Command signature layout changed");
            Require(stored.pArgumentDescs != &argument, "Command signature retained caller-owned argument storage");
            auto query = Method<decltype(&CommandSignature::QueryInterface)>(signature.get(), 0);
            for (const auto& id : {CommandSignatureId, UnknownId, ObjectId, DeviceChildId, PageableId}) {
                void* identity = nullptr;
                Require(query(signature.get(), id, &identity) == S_OK && identity == signature.get(), "Command signature COM identity changed");
                Require(Method<decltype(&CommandSignature::Release)>(signature.get(), 2)(static_cast<CommandSignature*>(identity)) == 1, "Command signature interface reference was not owned");
            }
            void* parent = nullptr;
            Require(Method<decltype(&CommandSignature::GetDevice)>(signature.get(), 7)(signature.get(), XboxDeviceId, &parent) == S_OK && parent == device, "Command signature parent identity changed");
            Method<decltype(&Device::Release)>(device, 2)(static_cast<Device*>(parent));
            Guid key{1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}};
            std::uint32_t value = 123;
            Require(Method<decltype(&CommandSignature::SetPrivateData)>(signature.get(), 4)(signature.get(), key, sizeof(value), &value) == S_OK, "Command signature private data was not stored");
            std::uint32_t copied = 0;
            std::uint32_t bytes = sizeof(copied);
            Require(Method<decltype(&CommandSignature::GetPrivateData)>(signature.get(), 3)(signature.get(), key, &bytes, &copied) == S_OK && copied == value, "Command signature private data changed");
            Require(Method<decltype(&CommandSignature::SetName)>(signature.get(), 6)(signature.get(), L"Indirect command signature") == S_OK, "Command signature name was not stored");
        }
    }
    IndirectArgumentDescription arguments[3]{};
    arguments[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
    arguments[0].Parameter = 3;
    arguments[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
    arguments[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    CommandSignatureDescription description{52, 3, arguments, 0};
    auto signature = MakeCommandSignature(device, description);
    Require(signature->GetDescription().NumArgumentDescs == 3, "Indirect buffer bindings were not retained");
    CheckIndirectRoots(device);
    CheckIndirectInvalid(device);
}
