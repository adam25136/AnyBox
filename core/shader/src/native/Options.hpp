#pragma once

#include <Translation/InstructionTranslator.hpp>
#include <SpirvBackend/SpirvEmitter.hpp>

namespace AnyBox::Shader {
ShaderRecompiler::TranslateOptions MakeNativeOptions(ShaderRecompiler::ShaderStageKind stage, std::uint32_t waveSize, std::uint32_t userRegisters, const ShaderRecompiler::ShaderFloatMode& floatMode, const ShaderRecompiler::SpirvTargetOptions& target);
}
