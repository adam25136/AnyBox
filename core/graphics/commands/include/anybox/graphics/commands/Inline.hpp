#pragma once

#include "RootState.hpp"
#include <array>
#include <variant>

namespace AnyBox::Graphics::Commands {
struct RootWrite {
    std::uint32_t parameter;
    std::uint32_t offset;
    std::span<const std::uint32_t> words;
};

struct Dispatch {
    std::array<std::uint32_t, 3> groups;
    std::uint32_t header;
};

inline constexpr std::uint32_t PipelineHeader = 0x8AB00007;
inline constexpr std::size_t PipelineWordCount = 7;

struct PipelineBind {
    std::uint64_t address;
};

using InlineCommand = std::variant<RootWrite, Dispatch, PipelineBind>;

std::uint32_t RootHeader(std::uint32_t parameter);

class InlineReader {
public:
    explicit InlineReader(std::span<const std::uint32_t> words) : words(words) {}
    bool Empty() const noexcept { return words.empty(); }
    InlineCommand Next(const RootState& root) { return Next(&root); }
    InlineCommand Next(const RootState* root = nullptr);

private:
    std::span<const std::uint32_t> words;
};
}
