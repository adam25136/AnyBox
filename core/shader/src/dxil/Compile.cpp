#include "ComputeCompiler.hpp"
#include "flow/Builder.hpp"
#include "Padding.hpp"
#include "flow/LaneReads.hpp"
#include "flow/predication/Plan.hpp"
#include "../backend/Spanned.hpp"
#include "../backend/WaveControl.hpp"

AnyBox::Shader::ComputeCompilation AnyBox::Shader::Dxil::CompileCompute(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    (void)Flow::Predication::Run(program, input);
    auto selected = target;
    if (UsesSpannedWave(input, selected) && !HasWave64Divergence(program, input)) selected.spannedWaves = false;
    auto structured = Flow::Builder(program).Run();
    Flow::ValidateLaneReads(structured, input);
    MaskComputePadding(structured, input);
    return AnyBox::Shader::CompileCompute(structured, input, selected, layout);
}
