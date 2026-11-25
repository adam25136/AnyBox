#include "Indirect.hpp"

void AnyBox::Graphics::Tests::CheckIndirectInvalid(Device* device) {
    IndirectArgumentDescription argument{};
    argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    CommandSignatureDescription description{12, 1, &argument, 0};
    auto create = Method<decltype(&Device::CreateCommandSignature)>(device, 41);
    void* output = device;
    Reject<std::invalid_argument>([&] { create(device, nullptr, nullptr, CommandSignatureId, &output); });
    Require(!output, "Rejected command signature retained a stale output");
    Reject<std::invalid_argument>([&] { create(nullptr, &description, nullptr, CommandSignatureId, &output); });
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, Guid{}, &output); });
    description.NumArgumentDescs = 0;
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    description.NumArgumentDescs = 16;
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    description.NumArgumentDescs = 1;
    description.ByteStride = 8;
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    description.ByteStride = 13;
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    description.ByteStride = 12;
    description.NodeMask = 2;
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    description.NodeMask = 0;
    argument.Type = XboxDispatchArgument;
    argument.Parameter = 1;
    Reject<std::runtime_error>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    argument.Parameter = 0;
    argument.Type = 999;
    Reject<std::runtime_error>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
    argument.Parameter = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    Reject<std::out_of_range>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    argument.Parameter = 0;
    description.ByteStride = 16;
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    IndirectArgumentDescription actions[2]{};
    actions[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
    actions[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    description = {28, 2, actions, 0};
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
    actions[0].Type = actions[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    description = {24, 2, actions, 0};
    Reject<std::invalid_argument>([&] { create(device, &description, nullptr, CommandSignatureId, &output); });
}
