#pragma once

#include "Compilation.hpp"
#include <Optimization/ShaderStageInputInfo.hpp>

namespace AnyBox::Shader {
StageCompilation CompilePixel(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderPixelInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
