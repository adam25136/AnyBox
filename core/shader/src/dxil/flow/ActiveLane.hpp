#pragma once

namespace ShaderRecompiler { class IrValue; }

namespace AnyBox::Shader::Dxil::Flow {
bool ReadsActiveLane(const ShaderRecompiler::IrValue& read, unsigned waveSize);
}
