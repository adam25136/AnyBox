#include <anybox/shader/PixelCompiler.hpp>
#include "Bindings.hpp"
#include "Validation.hpp"
#include <stdexcept>

AnyBox::Shader::StageCompilation AnyBox::Shader::CompilePixel(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderPixelInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    using namespace ShaderRecompiler;
    if (program.Resources().stage != IrShaderStage::Pixel) throw std::invalid_argument("Pixel compilation received a different shader stage");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::invalid_argument("Pixel shader wave size is invalid");
    if (program.WaveSize() != target.subgroupSize) throw std::runtime_error("Pixel shader execution on a different subgroup width is not implemented");
    if (input.inputNum > 32) throw std::invalid_argument("Pixel shader interpolator count exceeds its register limit");
    if (program.Info().scratchDwords != 0 && input.scratchSizeDwords != 0 && program.Info().scratchDwords != input.scratchSizeDwords) throw std::invalid_argument("Pixel shader scratch declarations disagree");
    if (program.Info().scratchDwords == 0) program.Info().scratchDwords = input.scratchSizeDwords;
    ShaderStageInputInfo stage{nullptr, &input, nullptr};
    StageCompilation result;
    result.bindings = PrepareShaderBindings(program, stage, target, layout);
    result.code = SpirvEmitter{}.Emit(program, stage, result.bindings, target, &result.info);
    ValidateSpirv(result.code, target.vulkanVersion);
    return result;
}
