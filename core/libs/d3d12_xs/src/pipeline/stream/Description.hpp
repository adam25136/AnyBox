#pragma once

#include <anybox/platform/Windows.hpp>
#include <d3d12.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace AnyBox::Graphics {
struct PipelineSubobjectSchema {
    std::size_t offset;
    std::size_t payload;
    std::size_t size;
};
PipelineSubobjectSchema GetPipelineSubobjectSchema(std::uint32_t type);
enum class PipelineStreamKind { Compute, Graphics, Mesh };
class PipelineStream {
public:
    explicit PipelineStream(std::span<const std::byte> bytes);
    bool Has(std::uint32_t type) const noexcept { return type < objects.size() && !objects[type].empty(); }
    template<class TValue>
    TValue Read(std::uint32_t type) const {
        static_assert(std::is_trivially_copyable_v<TValue>);
        if (!Has(type) || objects[type].size() != sizeof(TValue)) throw std::logic_error("Pipeline subobject payload has another type or is absent");
        TValue value;
        std::memcpy(&value, objects[type].data(), sizeof(value));
        return value;
    }
    D3D12_SHADER_BYTECODE GetShader(std::uint32_t type) const;
    PipelineStreamKind GetKind() const;
    D3D12_COMPUTE_PIPELINE_STATE_DESC GetComputeDescription() const;

private:
    std::array<std::span<const std::byte>, 25> objects{};
};
}
