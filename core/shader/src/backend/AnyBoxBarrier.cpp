#include "AnyBoxBarrier.hpp"
#include <SpirvBackend/SpirvEmitterInstructions.hpp>
#include <spirv/unified1/spirv.hpp>

void AnyBox::Shader::Backend::EmitDxilBarrier(ShaderRecompiler::SpirvValueEmitContext& context, const ShaderRecompiler::IrValue& instruction) {
    auto flags = instruction.Flags<std::uint64_t>();
    if (flags == 0) {
        ShaderRecompiler::EmitBarrier(context, instruction);
        return;
    }
    if ((flags & DxilBarrierTag) == 0 || (flags & ~(DxilBarrierTag | 15)) != 0 || (flags & 15) == 0) context.Fail(instruction, "has invalid DXIL barrier flags");
    auto& state = context.state;
    if (state.program.Resources().stage != ShaderRecompiler::IrShaderStage::Compute || state.laneCount != 1) context.Fail(instruction, "requires a native compute wave for a DXIL barrier");
    auto mode = static_cast<std::uint32_t>(flags & 15);
    auto scope = mode & 2 ? spv::ScopeDevice : spv::ScopeWorkgroup;
    std::uint32_t semantics = 0;
    if (mode & 6) semantics |= spv::MemorySemanticsUniformMemoryMask | spv::MemorySemanticsImageMemoryMask;
    if (mode & 8) semantics |= ShaderRecompiler::LdsMemorySemantics(state);
    if (semantics != 0) semantics |= spv::MemorySemanticsAcquireReleaseMask;
    auto memoryScope = ShaderRecompiler::ConstantU32(state, scope);
    auto memorySemantics = ShaderRecompiler::ConstantU32(state, semantics);
    if (mode & 1) state.module.AddFunction(spv::OpControlBarrier, ShaderRecompiler::ConstantU32(state, spv::ScopeWorkgroup), memoryScope, memorySemantics);
    else state.module.AddFunction(spv::OpMemoryBarrier, memoryScope, memorySemantics);
}
