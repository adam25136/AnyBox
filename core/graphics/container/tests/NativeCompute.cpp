#include "NativeFixtures.hpp"
#include <anybox/graphics/container/NativeCompute.hpp>
#include <array>

void CheckNativeComputeState() {
    using namespace Fixtures;
    using AnyBox::Graphics::Container::ReadNativeComputeState;
    auto data = MakeComputeProgram();
    auto state = ReadNativeComputeState(data);
    Require(state.threads == std::array<std::uint32_t, 3>{4, 2, 2});
    Require(state.declaredSharedBytes == 513 && state.floatControl == 0xC0);
    Require(state.dx10Clamp && !state.ieeeMode);
    for (unsigned flags = 0; flags < 4; ++flags) {
        auto selected = data;
        Set(selected, 0x114, flags);
        auto mode = ReadNativeComputeState(selected);
        Require(mode.dx10Clamp == ((flags & 1) != 0) && mode.ieeeMode == ((flags & 2) != 0));
    }
    for (unsigned flags = 4; flags < 256; ++flags) {
        auto invalid = data;
        Set(invalid, 0x114, flags);
        Reject<std::runtime_error>([&] { (void)ReadNativeComputeState(invalid); });
    }
    for (unsigned mode = 0; mode < 256; ++mode) {
        auto selected = data;
        Set(selected, 0x104, mode);
        Require(ReadNativeComputeState(selected).floatControl == mode);
    }
    auto invalidMode = data;
    Set(invalidMode, 0x104, 256);
    Reject<std::invalid_argument>([&] { (void)ReadNativeComputeState(invalidMode); });
    Require(state.resourceControl == ((2u << 15) | (2u << 11) | 0x380 | 8));
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 7>{{{0x100, 4}, {0x13C, 0}, {0x140, 0}, {0x144, 0}, {0x13C, UINT32_MAX}, {0x140, 1024}, {0x10C, 512}}}) {
        auto invalid = data;
        Set(invalid, offset, value);
        Reject<std::invalid_argument>([&] { (void)ReadNativeComputeState(invalid); });
    }
    Set(data, 0x10C, 1024);
    Require(ReadNativeComputeState(data).declaredSharedBytes == 1024);
    Set(data, 0x10C, 1025);
    Reject<std::invalid_argument>([&] { (void)ReadNativeComputeState(data); });
}
