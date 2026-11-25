#pragma once

namespace ShaderRecompiler { class IrProgram; struct ShaderComputeInputInfo; }

namespace AnyBox::Shader::Dxil {
void MaskComputePadding(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input);
}
