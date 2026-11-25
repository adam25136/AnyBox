#pragma once

#include "Native.hpp"
#include "PixelCompiler.hpp"
#include <anybox/graphics/container/NativeStage.hpp>
#include <Recompiler.hpp>

namespace AnyBox::Shader {
ShaderRecompiler::IrProgram LowerNativePixel(const NativeFrontend& frontend, const Graphics::Container::NativeStageState& state, const ShaderRecompiler::ShaderPixelInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target);
StageCompilation CompileNativePixel(std::span<const std::byte> program, const ShaderRecompiler::ShaderPixelInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
