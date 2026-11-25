#pragma once

#include "BufferArguments.hpp"
#include <anybox/graphics/container/RootSignature.hpp>

namespace AnyBox::Shader {
enum class RootArgumentKind : std::uint8_t { Buffer, Constants, Table };

struct RootArgumentRange {
    BufferBinding binding;
    RootArgumentKind kind;
    std::uint32_t parameter;
    std::uint32_t tableOffset;
    std::uint32_t firstUserRegister;
    std::uint32_t wordCount;
};

class RootArgumentPlan {
public:
    RootArgumentPlan(std::span<const BufferBinding> bindings, const Graphics::Container::RootSignatureLayout& root, std::uint32_t firstUserRegister = 0, std::uint32_t visibility = 0);
    std::span<const RootArgumentRange> GetRanges() const;
    const RootArgumentRange& Find(const BufferBinding& binding) const;
    std::uint32_t GetUserRegisterCount() const;

private:
    std::vector<RootArgumentRange> ranges;
    std::uint32_t userRegisterCount;
};
}
