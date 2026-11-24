#include "NativeFixtures.hpp"
#include <anybox/graphics/container/View.hpp>
#include <anybox/shader/Requirements.hpp>
#include <array>

namespace {
std::vector<std::byte> ContainerWithWaves(std::span<const std::byte> first, std::span<const std::byte> second) {
    using namespace AnyBox::Graphics::Container;
    auto source = Fixtures::MakeNativeContainer(first, second);
    auto program = DxbcView(source).FindPart(0x4C495844)->data;
    std::array<std::byte, 8> flags{};
    flags[1] = std::byte{0x40};
    return BuildDxbc(std::array<Part, 2>{{{0x4C495844, program}, {0x30494653, flags}}});
}
}

void CheckNativeWaveRequirements() {
    using namespace Fixtures;
    using AnyBox::Shader::GetComputeWaveSize;
    auto native32 = MakeComputeProgram();
    auto native64 = native32;
    Set(native64, 0xD4, 0);
    auto packed32 = PackNativeProgram(native32);
    auto packed64 = PackNativeProgram(native64);
    Require(GetComputeWaveSize(ContainerWithWaves(packed32, packed32)) == 32u);
    Require(GetComputeWaveSize(ContainerWithWaves(packed64, packed64)) == 64u);
    Reject<std::runtime_error>([&] { (void)GetComputeWaveSize(ContainerWithWaves(packed32, packed64)); });
    auto broken = packed64;
    broken.pop_back();
    Reject<std::invalid_argument>([&] { (void)GetComputeWaveSize(ContainerWithWaves(broken, packed64)); });
}
