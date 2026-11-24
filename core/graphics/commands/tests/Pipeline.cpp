#include <anybox/graphics/commands/Inline.hpp>
#include <stdexcept>

namespace {
void Require(bool value) {
    if (!value) throw std::runtime_error("Inline pipeline test failed");
}

void Reject(std::span<const std::uint32_t> words) {
    AnyBox::Graphics::Commands::InlineReader reader(words);
    try { (void)reader.Next(); }
    catch (const std::exception&) {
        Require(reader.Empty() == words.empty());
        return;
    }
    throw std::runtime_error("Invalid inline pipeline was accepted");
}
}

void CheckInlinePipelines() {
    using namespace AnyBox::Graphics::Commands;
    std::array<std::uint32_t, 11> words{PipelineHeader, 0x12345010, 2, 0, 0, 0, 0, 0x4002C400, 1, 2, 3};
    InlineReader reader(words);
    Require(std::get<PipelineBind>(reader.Next()).address == 0x212345010);
    Require(std::get<Dispatch>(reader.Next()).groups == std::array<std::uint32_t, 3>{1, 2, 3});
    Require(reader.Empty());
    for (unsigned size = 0; size < 7; ++size) Reject(std::span(words).first(size));
    for (unsigned index = 3; index < 7; ++index) {
        auto invalid = words;
        invalid[index] = 1;
        Reject(invalid);
    }
    auto invalid = words;
    invalid[1] = invalid[2] = 0;
    Reject(invalid);
    invalid = words;
    invalid[1] |= 1;
    Reject(invalid);
    invalid = words;
    invalid[0] += 1;
    Reject(invalid);
    std::array root{RootHeader(0), 1u};
    Reject(root);
}
