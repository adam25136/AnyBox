#pragma once

#include "Tests.hpp"
#include "../../src/pipeline/stream/Description.hpp"

namespace AnyBox::Graphics::Tests {
class StreamFixture {
public:
    template<class TValue>
    void Append(std::uint32_t type, const TValue& value) {
        struct Subobject { std::uint32_t type; TValue value; };
        Subobject object{type, value};
        auto offset = data.size();
        data.resize(offset + (sizeof(object) + 7) / 8);
        std::memcpy(data.data() + offset, &object, sizeof(object));
    }
    std::span<const std::byte> GetBytes() const noexcept { return std::as_bytes(std::span(data)); }
    D3D12_PIPELINE_STATE_STREAM_DESC GetDescription() noexcept { return {data.size() * 8, data.data()}; }

private:
    std::vector<std::uint64_t> data;
};
void CheckPipelineStreamParser();
}
