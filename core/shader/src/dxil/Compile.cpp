#include "ComputeCompiler.hpp"
#include "flow/Builder.hpp"
#include "Padding.hpp"
#include "flow/LaneReads.hpp"
#include "flow/predication/Plan.hpp"

AnyBox::Shader::ComputeCompilation AnyBox::Shader::Dxil::CompileCompute(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    (void)Flow::Predication::Run(program, input);
    auto structured = Flow::Builder(program).Run();
    Flow::ValidateLaneReads(structured, input);
    MaskComputePadding(structured, input);
    return AnyBox::Shader::CompileCompute(structured, input, target, layout);
}
