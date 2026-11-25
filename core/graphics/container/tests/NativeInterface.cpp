#include "NativeFixtures.hpp"
#include <anybox/graphics/container/NativeInterface.hpp>

namespace {
std::vector<std::byte> MakeInterface(std::uint32_t stage, std::uint32_t version) {
    auto data = Fixtures::MakeNativeProgram();
    data.resize(0x400);
    Fixtures::Set(data, 0x10, version);
    Fixtures::Set(data, 0x100, stage);
    Fixtures::Set(data, 0xC0, 0x300);
    Fixtures::Set(data, 0xC8, 0x100);
    for (std::size_t offset = 0x138; offset < 0x300; offset += 4) Fixtures::Set(data, offset, 0);
    return data;
}
}

void CheckNativeInterface() {
    using namespace AnyBox::Graphics::Container;
    using namespace Fixtures;
    for (auto version : {5u, 6u}) {
        auto shift = version == 6 ? 8u : 0u;
        auto pixel = MakeInterface(5, version);
        auto base = 0x130 + shift;
        Set(pixel, base, 0x000F0306);
        Set(pixel, base + 4, 0x01030000);
        Set(pixel, base + 8, 0x01050206);
        Set(pixel, base + 12, 0x02020000);
        Set(pixel, base + 0x100, 2);
        Set(pixel, base + 0x110, 0x302);
        auto inputs = ReadNativePixelInputs(pixel);
        Require(inputs.count == 2 && inputs.inputEnable == 0x302);
        Require(inputs.attributes[0].kind == 6 && inputs.attributes[0].registerIndex == 3 && inputs.attributes[0].componentMask == 15 && inputs.attributes[0].interpolation == 1);
        Require(inputs.attributes[1].registerIndex == 2 && inputs.attributes[1].componentMask == 5 && inputs.attributes[1].interpolation == 2);
        auto valid = pixel;
        Set(pixel, base + 0x100, 33);
        Reject<std::invalid_argument>([&] { ReadNativePixelInputs(pixel); });
        pixel = valid;
        Set(pixel, base + 0x110, 0x10000);
        Reject<std::runtime_error>([&] { ReadNativePixelInputs(pixel); });
        pixel = valid;
        pixel[base + 3] = std::byte{1};
        Reject<std::invalid_argument>([&] { ReadNativePixelInputs(pixel); });
        pixel = valid;
        pixel[base + 6] = std::byte{4};
        Reject<std::invalid_argument>([&] { ReadNativePixelInputs(pixel); });
        pixel = valid;
        pixel[base + 2] = std::byte{16};
        Reject<std::invalid_argument>([&] { ReadNativePixelInputs(pixel); });
        pixel = valid;
        pixel[base + 7] = std::byte{3};
        Reject<std::runtime_error>([&] { ReadNativePixelInputs(pixel); });
        pixel = valid;
        Set(pixel, 0xC0, 0x138);
        Reject<std::invalid_argument>([&] { ReadNativePixelInputs(pixel); });

        auto vertex = MakeInterface(4, version);
        base = 0x1D4 + shift;
        Set(vertex, base, 0x00000306);
        vertex[base + 6] = std::byte{6};
        vertex[base + 7] = std::byte{2};
        vertex[base + 8] = std::byte{1};
        Set(vertex, base + 0xC0, 2);
        auto outputs = ReadNativeVertexOutputs(vertex);
        Require(outputs.count == 2 && outputs.parameters[0].kind == 6 && outputs.parameters[0].registerIndex == 3 && outputs.parameters[1].registerIndex == 2);
        auto validVertex = vertex;
        Set(vertex, base + 0xC0, 33);
        Reject<std::invalid_argument>([&] { ReadNativeVertexOutputs(vertex); });
        vertex = validVertex;
        vertex[base + 8] = std::byte{0};
        Reject<std::invalid_argument>([&] { ReadNativeVertexOutputs(vertex); });
        Reject<std::invalid_argument>([&] { ReadNativePixelInputs(validVertex); });
        Reject<std::invalid_argument>([&] { ReadNativeVertexOutputs(valid); });
        Set(validVertex, 0x10, 7);
        Reject<std::runtime_error>([&] { ReadNativeVertexOutputs(validVertex); });
    }
}
