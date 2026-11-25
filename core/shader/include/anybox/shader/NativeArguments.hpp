#pragma once

#include "NativeCompute.hpp"
#include "RootArguments.hpp"
#include <anybox/graphics/container/NativeBindings.hpp>

namespace AnyBox::Shader {
void BindNativeRootArguments(NativeComputeTranslation& translation, std::span<const Graphics::Container::NativeRootBinding> roots, const RootArgumentPlan& plan);
void BindNativeGraphicsRootArguments(ShaderRecompiler::IrProgram& program, std::span<const Graphics::Container::NativeRootBinding> roots, const RootArgumentPlan& plan);
}
