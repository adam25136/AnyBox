#pragma once

#include <anybox/buffer/AddressSpace.hpp>
#include <BdaAbi.hpp>
#include <cstddef>
#include <span>

namespace AnyBox::Shader::Runtime {
struct MemoryBinding {
    std::shared_ptr<const Graphics::Buffers::Mapping> mapping;
    std::uint32_t permissions = ShaderRecompiler::BdaAbi::Read | ShaderRecompiler::BdaAbi::Write;
};

class MemoryTable {
public:
    explicit MemoryTable(std::span<const MemoryBinding> bindings);
    std::span<const ShaderRecompiler::BdaAbi::Range> GetRanges() const noexcept { return ranges; }
    std::size_t GetByteSize() const noexcept;
    void Write(std::span<std::byte> destination) const;

private:
    std::vector<MemoryBinding> bindings;
    std::vector<ShaderRecompiler::BdaAbi::Range> ranges;
};
}
