#include "Bindings.hpp"
#include "Validation.hpp"
#include "choices/Choice.hpp"
#include <Optimization/ConstantFolder.hpp>
#include <Optimization/DeadCodeEliminator.hpp>
#include <Optimization/ResourceTracker.hpp>
#include <Optimization/ResourceMaterializer.hpp>
#include <Optimization/RedundantValueEliminator.hpp>
#include <Optimization/ShaderInfoCollector.hpp>
#include <Optimization/SrtWalker.hpp>
#include <spirv/unified1/spirv.hpp>
#include <algorithm>
#include <limits>
#include <utility>

ShaderRecompiler::CompiledBindingLayout AnyBox::Shader::PrepareShaderBindings(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderStageInputInfo& stage, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    using namespace ShaderRecompiler;
    if (program.Resources().srtPlanComplete || program.Resources().resourceTrackingComplete || program.Metadata().shaderInfoComplete || program.Metadata().bindingLayoutComplete) throw std::logic_error("Shader binding preparation requires an unplanned program");
    ValidateProgram(program, true);
    for (const auto& block : program.Blocks()) {
        for (auto iterator = block->Instructions().begin(); iterator != block->Instructions().end();) {
            auto* instruction = *iterator++;
            auto opcode = instruction->Opcode();
            if (opcode == IrOpcode::Branch || opcode == IrOpcode::BranchConditional || opcode == IrOpcode::Return) block->RemoveInstruction(instruction);
        }
    }
    ConstantFolder{}.Fold(program);
    ResolveControlFlowIdentities(program);
    DeadCodeEliminator{}.RemoveIdentities(program);
    DeadCodeEliminator{}.Eliminate(program);
    if (PlanDescriptorChoices(program) != 0) DeadCodeEliminator{}.Eliminate(program);
    ValidateProgram(program, true);
    SrtWalker{}.BuildPlan(program);
    ResourceTracker{}.Track(program);
    auto nativeOffsets = target.nonConstantImageOffsets && std::ranges::find(target.supportedCapabilities, static_cast<std::uint32_t>(spv::CapabilityImageGatherExtended)) != target.supportedCapabilities.end();
    ResourceMaterializer{}.ApplyStaticInterface(program, nativeOffsets);
    DeadCodeEliminator{}.RemoveIdentities(program);
    (void)RedundantValueEliminator{}.Eliminate(program);
    DeadCodeEliminator{}.Eliminate(program);
    ShaderInfoCollector{}.Collect(program, stage);
    auto allocation = BindingAllocator{}.Allocate(program, layout, target.registrationModules);
    return std::move(static_cast<CompiledBindingLayout&>(allocation));
}
