#include "AnyBoxUniform.hpp"
#include <SpirvBackend/SpirvEmitterInstructions.hpp>
#include <spirv/unified1/spirv.hpp>

std::uint32_t AnyBox::Shader::Backend::EmitFirstLane(ShaderRecompiler::SpirvValueEmitContext& context, const ShaderRecompiler::IrValue& instruction) {
    auto& state = context.state;
    const auto* active = instruction.Argument(1)->Resolve();
    if (state.program.Resources().stage != ShaderRecompiler::IrShaderStage::Compute || state.laneCount != 1 || state.hostSubgroupSize != state.program.WaveSize() || !active->HasImmediate() || !active->ImmediateBool()) return ShaderRecompiler::EmitReadFirstLane(context, instruction);
    auto result = state.module.AllocateId();
    state.module.AddFunction(spv::OpGroupNonUniformBroadcastFirst, ShaderRecompiler::TypeU32(state), result, ShaderRecompiler::ConstantU32(state, spv::ScopeSubgroup), context.Arg(instruction, 0));
    return result;
}
