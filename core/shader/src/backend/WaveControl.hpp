#pragma once

namespace ShaderRecompiler { class IrProgram; class IrValue; struct ShaderComputeInputInfo; }

namespace AnyBox::Shader {
void ValidateWave64Control(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input);
bool IsWaveUniform(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrValue& value, const ShaderRecompiler::ShaderComputeInputInfo& input);
}
