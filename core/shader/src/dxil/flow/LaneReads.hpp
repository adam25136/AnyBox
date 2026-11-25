#pragma once

namespace ShaderRecompiler { class IrProgram; struct ShaderComputeInputInfo; }

namespace AnyBox::Shader::Dxil::Flow {
void ValidateLaneReads(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input);
}
