#pragma once

#include "Native.hpp"
#include "ComputeCompiler.hpp"
#include <anybox/graphics/container/NativeCompute.hpp>
#include <IntermediateRepresentation/IrProgram.hpp>
#include <SpirvBackend/SpirvEmitter.hpp>
#include <Optimization/ShaderStageInputInfo.hpp>

namespace ShaderRecompiler { struct ShaderFloatMode; }

namespace AnyBox::Shader {
struct NativeComputeTranslation {
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::ShaderComputeInputInfo input;
    bool rootArgumentsBound = false;
};

NativeComputeTranslation LowerNativeCompute(const NativeFrontend& frontend, const Graphics::Container::NativeComputeState& state, const ShaderRecompiler::ShaderFloatMode& floatMode, const ShaderRecompiler::SpirvTargetOptions& target);
ComputeCompilation CompileNativeCompute(const NativeFrontend& frontend, const Graphics::Container::NativeComputeState& state, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
