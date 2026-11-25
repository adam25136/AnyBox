#pragma once

#include <anybox/platform/Windows.hpp>
#include <d3d12.h>
#include <cstddef>
#include <cstdint>

namespace AnyBox::Graphics {
inline constexpr std::uint32_t XboxDispatchArgument = 105;
struct IndirectArgumentDescription {
    std::uint32_t Type;
    std::uint32_t Parameter;
    std::uint32_t Offset;
    std::uint32_t Count;
};
struct CommandSignatureDescription {
    std::uint32_t ByteStride;
    std::uint32_t NumArgumentDescs;
    const IndirectArgumentDescription* pArgumentDescs;
    std::uint32_t NodeMask;
};
static_assert(sizeof(IndirectArgumentDescription) == sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
static_assert(offsetof(IndirectArgumentDescription, Parameter) == offsetof(D3D12_INDIRECT_ARGUMENT_DESC, Constant));
static_assert(sizeof(CommandSignatureDescription) == sizeof(D3D12_COMMAND_SIGNATURE_DESC));
static_assert(offsetof(CommandSignatureDescription, pArgumentDescs) == offsetof(D3D12_COMMAND_SIGNATURE_DESC, pArgumentDescs));
static_assert(offsetof(CommandSignatureDescription, NodeMask) == offsetof(D3D12_COMMAND_SIGNATURE_DESC, NodeMask));
}
