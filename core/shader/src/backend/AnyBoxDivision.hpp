#pragma once

#include <cstdint>

namespace ShaderRecompiler { struct SpirvEmitterState; }

namespace AnyBox::Shader::Backend {
std::uint32_t EmitDivision(ShaderRecompiler::SpirvEmitterState& state, std::uint32_t numerator, std::uint32_t denominator);
}
