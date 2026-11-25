#pragma once

namespace ShaderRecompiler { class IrBuilder; class IrValue; }

namespace AnyBox::Shader::Dxil {
ShaderRecompiler::IrValue& SumWaveWords(ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrValue& source, ShaderRecompiler::IrValue& ballot, unsigned waveSize, bool prefix);
ShaderRecompiler::IrValue& CountWaveLanes(ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrValue& ballot, unsigned waveSize, bool prefix);
}
