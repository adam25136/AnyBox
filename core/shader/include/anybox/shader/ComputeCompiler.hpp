#pragma once

#include "Compilation.hpp"

namespace AnyBox::Shader {
using ComputeCompilation = StageCompilation;

ComputeCompilation CompileCompute(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
