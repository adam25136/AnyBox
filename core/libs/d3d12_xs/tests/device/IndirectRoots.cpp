#include "Indirect.hpp"
#include <d3d12_xs.hpp>

void AnyBox::Graphics::Tests::CheckIndirectRoots(Device* device) {
    for (auto version : {D3D_ROOT_SIGNATURE_VERSION_1_0, D3D_ROOT_SIGNATURE_VERSION_1_1}) {
        D3D12_ROOT_PARAMETER parameters[4]{};
        D3D12_ROOT_PARAMETER1 parameters1[4]{};
        for (unsigned index = 0; index < 4; ++index) {
            auto type = index == 0 ? D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS : index == 1 ? D3D12_ROOT_PARAMETER_TYPE_CBV : index == 2 ? D3D12_ROOT_PARAMETER_TYPE_SRV : D3D12_ROOT_PARAMETER_TYPE_UAV;
            parameters[index].ParameterType = type;
            parameters1[index].ParameterType = type;
            if (index != 0) {
                parameters[index].Descriptor.ShaderRegister = index;
                parameters1[index].Descriptor.ShaderRegister = index;
            }
        }
        parameters[0].Constants.Num32BitValues = 4;
        parameters1[0].Constants.Num32BitValues = 4;
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDescription{};
        rootDescription.Version = version;
        if (version == D3D_ROOT_SIGNATURE_VERSION_1_0) rootDescription.Desc_1_0 = {4, parameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE};
        else rootDescription.Desc_1_1 = {4, parameters1, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ID3DBlob* blob = nullptr;
        Require(D3D12SerializeVersionedRootSignature(&rootDescription, &blob, nullptr) == S_OK && blob, "Indirect root serialization failed");
        void* output = nullptr;
        auto result = Method<decltype(&Device::CreateRootSignature)>(device, 16)(device, 0, blob->GetBufferPointer(), blob->GetBufferSize(), RootSignatureId, &output);
        blob->Release();
        Require(result == S_OK && output, "Indirect root creation failed");
        auto* root = static_cast<RootSignature*>(output);
        auto rootRelease = Method<decltype(&RootSignature::Release)>(root, 2);
        IndirectArgumentDescription arguments[5]{};
        arguments[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        arguments[0].Parameter = 0;
        arguments[0].Offset = 1;
        arguments[0].Count = 3;
        for (unsigned index = 1; index < 4; ++index) {
            arguments[index].Type = index == 1 ? D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW : index == 2 ? D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW : D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
            arguments[index].Parameter = index;
        }
        arguments[4].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
        CommandSignatureDescription description{48, 5, arguments, 0};
        auto create = Method<decltype(&Device::CreateCommandSignature)>(device, 41);
        auto signature = MakeCommandSignature(device, description, root);
        Require(signature->GetRoot() == root && rootRelease(root) == 1, "Command signature did not retain its root signature");
        void* rejected = root;
        arguments[0].Count = 4;
        Reject<std::out_of_range>([&] { create(device, &description, root, CommandSignatureId, &rejected); });
        Require(!rejected, "Rejected indirect root layout retained an output");
        arguments[0].Count = 3;
        arguments[1].Parameter = 4;
        Reject<std::out_of_range>([&] { create(device, &description, root, CommandSignatureId, &rejected); });
        arguments[1].Parameter = 2;
        Reject<std::invalid_argument>([&] { create(device, &description, root, CommandSignatureId, &rejected); });
        arguments[1].Parameter = 1;
        Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &rejected); });
    }
}
