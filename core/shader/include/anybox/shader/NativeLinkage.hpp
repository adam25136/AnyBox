#pragma once

#include <anybox/graphics/container/NativeInterface.hpp>
#include <Optimization/ShaderStageInputInfo.hpp>

namespace AnyBox::Shader {
void LinkNativePixelAttributes(ShaderRecompiler::ShaderPixelInputInfo& input, const Graphics::Container::NativePixelInputs& pixel, const Graphics::Container::NativeVertexOutputs& vertex);
}
