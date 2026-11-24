#include "AnyBoxDivision.hpp"
#include <SpirvBackend/SpirvEmitterHelpers.hpp>
#include <spirv/unified1/spirv.hpp>

std::uint32_t AnyBox::Shader::Backend::EmitDivision(ShaderRecompiler::SpirvEmitterState& state, std::uint32_t numerator, std::uint32_t denominator) {
    auto result = state.module.AllocateId();
    state.module.AddFunction(spv::OpFDiv, ShaderRecompiler::TypeF32(state), result, numerator, denominator);
    state.module.AddAnnotation(spv::OpDecorate, result, spv::DecorationNoContraction);
    return result;
}
