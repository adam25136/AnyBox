#include <anybox/shader/NativeCompute.hpp>
#include <Recompiler.hpp>

AnyBox::Shader::ComputeCompilation AnyBox::Shader::CompileNativeCompute(const NativeFrontend& frontend, const Graphics::Container::NativeComputeState& state, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    ShaderRecompiler::ShaderFloatMode mode{state.floatControl, state.dx10Clamp, state.ieeeMode, false};
    auto lowered = LowerNativeCompute(frontend, state, mode, target);
    return CompileCompute(lowered.program, lowered.input, target, layout);
}
