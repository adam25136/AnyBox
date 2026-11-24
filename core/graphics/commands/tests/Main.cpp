#include <anybox/graphics/commands/Inline.hpp>
#include <anybox/ExceptionTest.hpp>
#include <algorithm>
#include <stdexcept>

using namespace AnyBox::Graphics::Commands;
using namespace AnyBox::Graphics::Container;

void CheckInlinePipelines();

namespace {
void Require(bool value) {
    if (!value) throw std::runtime_error("Command argument test failed");
}

template<typename TException, typename TFunction>
void Reject(TFunction&& function) {
    try { function(); }
    catch (const TException&) { return; }
    throw std::runtime_error("Command argument test accepted an invalid operation");
}

RootSignatureLayout Layout() {
    RootSignatureLayout layout{};
    layout.version = 2;
    RootParameter parameter{};
    parameter.kind = RootParameterKind::UnorderedAccess;
    layout.parameters.push_back(parameter);
    parameter.kind = RootParameterKind::Constants;
    parameter.wordCount = 14;
    layout.parameters.push_back(parameter);
    parameter.kind = RootParameterKind::Table;
    layout.parameters.push_back(parameter);
    return layout;
}

void CheckState() {
    RootState state(Layout());
    Require(state.GetWordCount(0) == 2 && state.GetWordCount(1) == 14 && state.GetWordCount(2) == 1);
    Reject<std::logic_error>([&] { state.Read(1, 0, 1); });
    std::array first{11u, 12u, 13u};
    state.Write(1, 2, first);
    Require(std::ranges::equal(state.Read(1, 2, 3), first));
    Reject<std::logic_error>([&] { state.Read(1, 1, 4); });
    std::array second{21u, 22u};
    state.Write(1, 3, second);
    std::array expected{11u, 21u, 22u};
    Require(std::ranges::equal(state.Read(1, 2, 3), expected));
    state.Write(1, 3, state.Read(1, 2, 3));
    Require(std::ranges::equal(state.Read(1, 3, 3), expected));
    state.Write(0, 0, second);
    state.Write(2, 0, std::span(first).first(1));
    Require(state.Read(0, 0, 2)[1] == 22 && state.Read(2, 0, 1)[0] == 11);
    Reject<std::invalid_argument>([&] { state.Write(0, 0, std::span(first).first(1)); });
    Reject<std::out_of_range>([&] { state.Write(1, 13, second); });
    Reject<std::out_of_range>([&] { state.Read(1, UINT32_MAX, 2); });
    Reject<std::out_of_range>([&] { state.GetKind(3); });
    state.Invalidate();
    Reject<std::logic_error>([&] { state.Read(0, 0, 2); });
    Require(state.Read(1, 14, 0).empty());
    auto invalid = Layout();
    invalid.parameters[1].wordCount = UINT32_MAX;
    Reject<std::length_error>([&] { RootState selected(invalid); });
    invalid.parameters[1].wordCount = 0;
    Reject<std::invalid_argument>([&] { RootState selected(invalid); });
}

void CheckStream() {
    RootState state(Layout());
    std::array words{RootHeader(0), 0xABCDEF00u, 1u, RootHeader(1) + (2u << 16) + 8u, 3u, 4u, 5u, RootHeader(2), 0x12340000u, 0x4002C400u, 7u, 8u, 9u};
    InlineReader reader(words);
    for (unsigned index = 0; index < 3; ++index) {
        auto command = reader.Next(state);
        auto update = std::get<RootWrite>(command);
        Require(update.parameter == index);
        state.Write(update.parameter, update.offset, update.words);
    }
    Require(state.Read(0, 0, 2)[0] == 0xABCDEF00 && state.Read(1, 2, 3)[2] == 5 && state.Read(2, 0, 1)[0] == 0x12340000);
    auto dispatch = std::get<Dispatch>(reader.Next(state));
    Require(dispatch.groups == std::array{7u, 8u, 9u} && dispatch.header == 0x4002C400 && reader.Empty());
    Reject<std::out_of_range>([&] { reader.Next(state); });
    for (auto header : {0x4002C400u, 0x4002E800u, 0x4002E804u}) {
        std::array record{header, 0u, UINT32_MAX, 1u};
        InlineReader selected(record);
        Require(std::get<Dispatch>(selected.Next(state)).header == header);
        for (unsigned size = 1; size < 4; ++size) {
            InlineReader truncated{std::span(record).first(size)};
            Reject<std::invalid_argument>([&] { truncated.Next(state); });
            Require(!truncated.Empty());
        }
    }
    auto reject = [&](std::initializer_list<std::uint32_t> record) {
        InlineReader selected{std::span(record.begin(), record.size())};
        Reject<std::exception>([&] { selected.Next(state); });
        Require(!selected.Empty());
    };
    reject({RootHeader(0), 1});
    reject({RootHeader(0) + 4, 1, 2});
    reject({RootHeader(2) + (1u << 16), 1, 2});
    reject({RootHeader(1) + (14u << 16), 1});
    reject({RootHeader(3), 1});
    reject({RootHeader(1) | 1u, 1});
    reject({0x4002C404, 1, 2, 3});
    reject({0xC0400000, 1});
    Reject<std::out_of_range>([] { RootHeader(64); });
}
}

int main() {
    return AnyBox::Tests::Run([] { CheckState(); CheckStream(); CheckInlinePipelines(); });
}
