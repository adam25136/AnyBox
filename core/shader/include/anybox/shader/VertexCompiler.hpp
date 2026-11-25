#pragma once

#include "Compilation.hpp"
#include <Optimization/ShaderStageInputInfo.hpp>

namespace AnyBox::Shader {
StageCompilation CompileVertex(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderVertexInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
