#pragma once

namespace ShaderRecompiler { class IrProgram; struct ShaderComputeInputInfo; }

namespace AnyBox::Shader {
void ValidateWave64Control(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input);
}
