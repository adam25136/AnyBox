#pragma once

#include "RootArguments.hpp"
#include <anybox/graphics/commands/RootState.hpp>

namespace AnyBox::Shader {
struct RootBufferValue {
    std::uint64_t address;
    std::uint64_t byteLength;
};

class IRootResources {
public:
    virtual ~IRootResources() = default;
    virtual RootBufferValue ResolveDescriptor(const BufferBinding& binding, std::uint64_t address) = 0;
    virtual RootBufferValue ResolveTable(const BufferBinding& binding, std::uint32_t tableAddress, std::uint32_t descriptorOffset) = 0;
};

class RootArgumentWriter {
public:
    RootArgumentWriter(const RootArgumentPlan& plan, std::span<const std::uint32_t> registers);
    void Write(const Graphics::Commands::RootState& state, IRootResources& resources, std::span<std::uint32_t> destination) const;
    std::uint32_t GetWordCount() const noexcept { return wordCount; }
    bool RequiresVolatileTables(const Graphics::Container::RootSignatureLayout& root) const;

private:
    struct Copy {
        std::uint32_t source;
        std::uint32_t destination;
    };
    struct Step {
        RootArgumentRange range;
        std::uint32_t element;
        std::vector<Copy> copies;
    };
    std::vector<Step> steps;
    std::uint32_t wordCount;
};
}
