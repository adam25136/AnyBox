#include "NativeFixtures.hpp"
#include <anybox/graphics/container/NativeBindings.hpp>
#include <array>

namespace {
using namespace Fixtures;
using namespace AnyBox::Graphics::Container;

std::vector<std::byte> MakeBindings() {
    auto data = MakeComputeProgram();
    data.resize(0x240);
    Set(data, 0xE8, 0x1F0);
    Set(data, 0xF0, 0x204);
    Set(data, 0xF8, 1);
    Set(data, 0xFC, 1);
    Set(data, 0x200, 1);
    Set(data, 0x204, 7);
    Set(data, 0x208, 2);
    Set(data, 0x20C, 9);
    Set(data, 0x210, 0x325);
    Set(data, 0x214, 0x103);
    Set(data, 0x218, 7);
    Set(data, 0x21C, 9);
    Set(data, 0x220, 1);
    Set(data, 0x224, 0x32504);
    return data;
}
}

void CheckNativeBindings() {
    auto data = MakeBindings();
    auto bindings = ReadNativeComputeBindings(data);
    Require(bindings.roots.size() == 1 && bindings.resources.size() == 1);
    const auto& root = bindings.roots.front();
    Require(root.kind == NativeRootKind::Descriptor && root.tableOffset == 7);
    Require(root.registerCount == 2 && root.shaderRegister == 9);
    Require(root.firstUserRegister == 37 && root.rootParameter == 3);
    const auto& resource = bindings.resources.front();
    Require(resource.kind == NativeResourceKind::UnorderedAccess && resource.rootKind == NativeRootKind::Descriptor);
    Require(resource.tableOffset == 7 && resource.firstShaderRegister == 9 && resource.stride == 1);
    Require(resource.descriptorWords == 4 && resource.firstUserRegister == 37 && resource.rootParameter == 3);
    for (auto stage : {4u, 5u, 6u}) {
        auto graphics = data;
        Set(graphics, 0x100, stage);
        auto result = ReadNativeBindings(graphics);
        Require(result.roots.size() == 1 && result.resources.size() == 1);
        Require(result.roots[0].registerCount == root.registerCount && result.resources[0].stride == resource.stride);
    }

    for (std::uint32_t kind : {0u, 1u, 3u, 4u, 5u, 6u}) {
        auto selected = data;
        Set(selected, 0x200, kind);
        Set(selected, 0x214, (kind << 8) | 3);
        auto result = ReadNativeComputeBindings(selected);
        Require(static_cast<std::uint32_t>(result.roots.front().kind) == kind);
        Require(static_cast<std::uint32_t>(result.resources.front().rootKind) == kind);
    }
    for (std::uint32_t kind = 0; kind < 4; ++kind) {
        auto selected = data;
        Set(selected, 0x214, 0x100 | kind);
        Require(static_cast<std::uint32_t>(ReadNativeComputeBindings(selected).resources.front().kind) == kind);
    }
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 12>{{{0xE8, 0x1F1}, {0xE8, 0x130}, {0xE8, 0x100}, {0xE8, 0x240}, {0xEC, UINT32_MAX}, {0xF4, 1}, {0xF8, UINT32_MAX}, {0xFC, UINT32_MAX}, {0xF0, 0x1F0}, {0xF0, 0x1F4}, {0xF0, 0x1EC}, {0x100, 4}}}) {
        auto invalid = data;
        Set(invalid, offset, value);
        Reject<std::invalid_argument>([&] { (void)ReadNativeComputeBindings(invalid); });
    }
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 7>{{{0x200, 2}, {0x200, UINT32_MAX}, {0x210, 0x10325}, {0x214, 4}, {0x214, 0x203}, {0x214, 0x10103}, {0x224, 0x1032504}}}) {
        auto invalid = data;
        Set(invalid, offset, value);
        Reject<std::runtime_error>([&] { (void)ReadNativeComputeBindings(invalid); });
    }
    for (std::size_t size = 0; size < 0x228; ++size) Reject<std::invalid_argument>([&] { (void)ReadNativeComputeBindings(std::span(data).first(size)); });
    auto empty = MakeComputeProgram();
    auto result = ReadNativeComputeBindings(empty);
    Require(result.roots.empty() && result.resources.empty());
}
