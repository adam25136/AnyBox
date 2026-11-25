#include "ComputeCompiler.hpp"
#include "flow/Builder.hpp"
#include "Padding.hpp"

AnyBox::Shader::ComputeCompilation AnyBox::Shader::Dxil::CompileCompute(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    auto structured = Flow::Builder(program).Run();
    MaskComputePadding(structured, input);
    return AnyBox::Shader::CompileCompute(structured, input, target, layout);
}
