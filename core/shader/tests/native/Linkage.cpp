#include <anybox/shader/NativeLinkage.hpp>
#include "NativeFixtures.hpp"

void CheckNativeLinkage() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    NativeVertexOutputs vertex;
    vertex.count = 3;
    vertex.parameters[0] = {6, 7};
    vertex.parameters[1] = {0, 2};
    vertex.parameters[2] = {6, 3};
    NativePixelInputs pixel;
    pixel.count = 2;
    pixel.attributes[0] = {6, 3, 15, 1};
    pixel.attributes[1] = {6, 7, 5, 2};
    ShaderPixelInputInfo input;
    input.targetOutputMode[0] = 4;
    input.psDepthExportEnable = true;
    LinkNativePixelAttributes(input, pixel, vertex);
    Require(input.inputNum == 2 && input.InputSlot(0) == 2 && input.InputSlot(1) == 0);
    Require(input.InputIsFlat(0) && !input.InputIsFlat(1));
    Require(input.targetOutputMode[0] == 4 && input.psDepthExportEnable);
    auto valid = pixel;
    pixel.attributes[0].registerIndex = 2;
    Reject<std::invalid_argument>([&] { LinkNativePixelAttributes(input, pixel, vertex); });
    Require(input.inputNum == 2 && input.InputSlot(0) == 2);
    pixel = valid;
    pixel.attributes[0].kind = 17;
    Reject<std::runtime_error>([&] { LinkNativePixelAttributes(input, pixel, vertex); });
    pixel = valid;
    pixel.attributes[0].interpolation = 3;
    Reject<std::runtime_error>([&] { LinkNativePixelAttributes(input, pixel, vertex); });
    pixel = valid;
    pixel.attributes[0].componentMask = 16;
    Reject<std::invalid_argument>([&] { LinkNativePixelAttributes(input, pixel, vertex); });
    pixel = valid;
    vertex.parameters[1] = vertex.parameters[2];
    Reject<std::invalid_argument>([&] { LinkNativePixelAttributes(input, pixel, vertex); });
    vertex.count = 33;
    Reject<std::invalid_argument>([&] { LinkNativePixelAttributes(input, pixel, vertex); });
    vertex.count = 0;
    pixel.count = 0;
    LinkNativePixelAttributes(input, pixel, vertex);
    Require(input.inputNum == 0 && input.interpolatorSettings[0] == 0);
}
