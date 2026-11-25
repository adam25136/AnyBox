#pragma once

#include <SpirvBackend/SpirvEmitter.hpp>
#include <Optimization/ShaderStageInputInfo.hpp>

namespace AnyBox::Shader {
ShaderRecompiler::CompiledBindingLayout PrepareShaderBindings(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderStageInputInfo& stage, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
