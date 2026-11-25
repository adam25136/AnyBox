#include <anybox/shader/NativeCompute.hpp>
#include <Recompiler.hpp>
#include <stdexcept>
#include <string>

AnyBox::Shader::ComputeCompilation AnyBox::Shader::CompileNativeCompute(const NativeFrontend& frontend, const Graphics::Container::NativeComputeState& state, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    ShaderRecompiler::ShaderFloatMode mode{state.floatControl, false, false, false};
    auto lowered = LowerNativeCompute(frontend, state, mode, target);
    auto result = CompileCompute(lowered.program, lowered.input, target, layout);
    for (unsigned flags = 1; flags < 8; ++flags) {
        mode.dx10Clamp = (flags & 1) != 0;
        mode.ieeeMode = (flags & 2) != 0;
        mode.fp16Overflow = (flags & 4) != 0;
        auto alternative = LowerNativeCompute(frontend, state, mode, target);
        auto compiled = CompileCompute(alternative.program, alternative.input, target, layout);
        bool sameBindings = compiled.bindings.layout == result.bindings.layout && compiled.bindings.pushConstantOffsetBytes == result.bindings.pushConstantOffsetBytes && compiled.bindings.pushConstantSizeBytes == result.bindings.pushConstantSizeBytes;
        bool sameInvocation = compiled.info.fullSubgroups == result.info.fullSubgroups && compiled.info.functionLdsBuffer == result.info.functionLdsBuffer && compiled.info.meshWindowBuffer == result.info.meshWindowBuffer && compiled.info.splitStateLanes == result.info.splitStateLanes && compiled.info.scratchPoolSlotDwords == result.info.scratchPoolSlotDwords && compiled.info.anySubgroupSize == result.info.anySubgroupSize;
        if (compiled.code != result.code || !sameBindings || !sameInvocation) throw std::runtime_error("Xbox native compute depends on unverified floating flags: dx10Clamp=" + std::to_string(mode.dx10Clamp) + " ieeeMode=" + std::to_string(mode.ieeeMode) + " fp16Overflow=" + std::to_string(mode.fp16Overflow));
    }
    return result;
}
