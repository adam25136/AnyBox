#pragma once

#include <cstdint>

namespace ShaderRecompiler { struct SpirvValueEmitContext; class IrValue; }

namespace AnyBox::Shader::Backend {
inline constexpr std::uint64_t DxilBarrierTag = std::uint64_t{1} << 63;
void EmitDxilBarrier(ShaderRecompiler::SpirvValueEmitContext& context, const ShaderRecompiler::IrValue& instruction);
}
