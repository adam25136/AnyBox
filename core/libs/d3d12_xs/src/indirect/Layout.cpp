#include "Signature.hpp"

namespace {
struct RootParameter {
    D3D12_ROOT_PARAMETER_TYPE type;
    std::uint32_t constants;
};
RootParameter GetParameter(const AnyBox::Graphics::RootSignature* root, std::uint32_t index) {
    if (!root) throw std::invalid_argument("Indirect root argument requires a root signature");
    const auto& description = root->GetDescription();
    auto read = [index](const auto& layout) {
        if (index >= layout.NumParameters) throw std::out_of_range("Indirect root argument parameter index is invalid");
        const auto& parameter = layout.pParameters[index];
        return RootParameter{parameter.ParameterType, parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS ? parameter.Constants.Num32BitValues : 0};
    };
    if (description.Version == D3D_ROOT_SIGNATURE_VERSION_1_0) return read(description.Desc_1_0);
    if (description.Version == D3D_ROOT_SIGNATURE_VERSION_1_1) return read(description.Desc_1_1);
    throw std::runtime_error("Indirect root signature version is not implemented");
}
std::uint64_t RootArgumentSize(const AnyBox::Graphics::IndirectArgumentDescription& argument, const AnyBox::Graphics::RootSignature* root) {
    auto parameter = GetParameter(root, argument.Parameter);
    if (argument.Type == D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT) {
        if (parameter.type != D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) throw std::invalid_argument("Indirect constants require a constants root parameter");
        auto count = argument.Count;
        auto offset = argument.Offset;
        if (count == 0 || offset > parameter.constants || count > parameter.constants - offset) throw std::out_of_range("Indirect constants exceed their root parameter");
        return static_cast<std::uint64_t>(count) * 4;
    }
    auto expected = argument.Type == D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW ? D3D12_ROOT_PARAMETER_TYPE_CBV : argument.Type == D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW ? D3D12_ROOT_PARAMETER_TYPE_SRV : D3D12_ROOT_PARAMETER_TYPE_UAV;
    if (parameter.type != expected) throw std::invalid_argument("Indirect descriptor does not match its root parameter");
    return 8;
}
}

void AnyBox::Graphics::ValidateIndirectLayout(const CommandSignatureDescription& description, const RootSignature* root) {
    if (!description.pArgumentDescs || description.NumArgumentDescs == 0 || description.NumArgumentDescs > 15) throw std::invalid_argument("Command signature argument array is invalid");
    if (description.ByteStride == 0 || description.ByteStride % 4 != 0 || description.NodeMask > 1) throw std::invalid_argument("Command signature stride or node mask is invalid");
    std::uint64_t size = 0;
    bool action = false;
    bool usesRoot = false;
    bool usesVertex = false;
    bool usesIndex = false;
    std::uint32_t actionType = 0;
    for (std::uint32_t index = 0; index < description.NumArgumentDescs; ++index) {
        const auto& argument = description.pArgumentDescs[index];
        if (action) throw std::invalid_argument("Indirect action must be the final argument");
        switch (argument.Type) {
        case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW: size += sizeof(D3D12_DRAW_ARGUMENTS); action = true; break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED: size += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS); action = true; break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
        case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH: size += sizeof(D3D12_DISPATCH_ARGUMENTS); action = true; break;
        case XboxDispatchArgument:
            if (argument.Parameter != 0) throw std::runtime_error("Xbox indirect dispatch command header mode is not implemented");
            size += sizeof(D3D12_DISPATCH_ARGUMENTS);
            action = true;
            break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
            if (argument.Parameter >= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) throw std::out_of_range("Indirect vertex buffer slot is invalid");
            usesVertex = true;
            size += sizeof(D3D12_VERTEX_BUFFER_VIEW);
            break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW: size += sizeof(D3D12_INDEX_BUFFER_VIEW); usesIndex = true; break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
        case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
        case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
        case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
            size += RootArgumentSize(argument, root);
            usesRoot = true;
            break;
        default: throw std::runtime_error("Indirect argument type is not implemented");
        }
        if (action) actionType = argument.Type;
    }
    if (!action || size > description.ByteStride) throw std::invalid_argument("Command signature has no terminal action or its arguments exceed the stride");
    if ((usesVertex && actionType != D3D12_INDIRECT_ARGUMENT_TYPE_DRAW && actionType != D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED) || (usesIndex && actionType != D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED)) throw std::invalid_argument("Indirect buffer bindings are incompatible with their terminal action");
    if (root && !usesRoot) throw std::invalid_argument("Command signature root signature is unnecessary");
}
