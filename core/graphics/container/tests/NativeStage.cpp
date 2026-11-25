#include "NativeFixtures.hpp"
#include <anybox/graphics/container/NativeStage.hpp>
#include <anybox/graphics/container/NativeCompute.hpp>

void CheckNativeStageState() {
    using namespace AnyBox::Graphics::Container;
    using namespace Fixtures;
    for (auto stage : {NativeHardwareStage::Vertex, NativeHardwareStage::Pixel, NativeHardwareStage::Compute}) {
        for (auto mode : {0u, 0xC0u, 255u}) {
            for (unsigned flags = 0; flags < 4; ++flags) {
                auto data = MakeComputeProgram();
                Set(data, 0x100, 0x20000 | static_cast<std::uint32_t>(stage));
                Set(data, 0x104, mode);
                Set(data, 0x114, flags);
                auto state = ReadNativeStageState(data);
                Require(state.stage == stage && state.floatControl == mode);
                Require(state.dx10Clamp == ((flags & 1) != 0) && state.ieeeMode == ((flags & 2) != 0));
                if (stage == NativeHardwareStage::Compute) {
                    auto compute = ReadNativeComputeState(data);
                    Require(compute.floatControl == state.floatControl && compute.dx10Clamp == state.dx10Clamp && compute.ieeeMode == state.ieeeMode);
                }
            }
        }
    }
    auto data = MakeComputeProgram();
    Set(data, 0x104, 256);
    Reject<std::invalid_argument>([&] { ReadNativeStageState(data); });
    Set(data, 0x104, 0xC0);
    Set(data, 0x114, 4);
    Reject<std::runtime_error>([&] { ReadNativeStageState(data); });
    Set(data, 0x114, 1);
    Set(data, 0x100, 3);
    Reject<std::runtime_error>([&] { ReadNativeStageState(data); });
    Reject<std::invalid_argument>([&] { ReadNativeStageState(std::span(data).first(0x110)); });
}
