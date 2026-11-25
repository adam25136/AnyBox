#include "Options.hpp"
#include <spirv/unified1/spirv.hpp>
#include <algorithm>

ShaderRecompiler::TranslateOptions AnyBox::Shader::MakeNativeOptions(ShaderRecompiler::ShaderStageKind stage, std::uint32_t waveSize, std::uint32_t userRegisters, const ShaderRecompiler::ShaderFloatMode& floatMode, const ShaderRecompiler::SpirvTargetOptions& target) {
    ShaderRecompiler::TranslateOptions options;
    options.stage = stage;
    options.waveSize = waveSize;
    options.userDataCount = userRegisters;
    options.floatMode = floatMode;
    auto supports = [&](spv::Capability capability) { return std::ranges::find(target.supportedCapabilities, static_cast<std::uint32_t>(capability)) != target.supportedCapabilities.end(); };
    options.nativeFloat16 = supports(spv::CapabilityFloat16) && supports(spv::CapabilityRoundingModeRTE) && supports(spv::CapabilityDenormPreserve);
    options.nativeFloat16Fma = options.nativeFloat16 && supports(spv::CapabilityFMAKHR);
    options.nativeInteger16 = supports(spv::CapabilityInt16);
    options.float64 = supports(spv::CapabilityFloat64);
    options.float32FlushToZero = target.float32FlushToZero;
    options.float32FlushDeclared = supports(spv::CapabilityDenormFlushToZero);
    options.float32DenormPreserve = target.float32DenormPreserve;
    return options;
}
