#pragma once

#include <cstdint>

namespace ShaderRecompiler { struct SpirvValueEmitContext; class IrValue; }

namespace AnyBox::Shader::Backend {
std::uint32_t EmitFirstLane(ShaderRecompiler::SpirvValueEmitContext& context, const ShaderRecompiler::IrValue& instruction);
}
