#include <anybox/shader/NativeVertex.hpp>
#include "Options.hpp"
#include "Optimize.hpp"
#include "Slices.hpp"
#include "Buffers.hpp"
#include <Optimization/SsaBuilder.hpp>

ShaderRecompiler::IrProgram AnyBox::Shader::LowerNativeVertex(const NativeFrontend& frontend, const Graphics::Container::NativeStageState& state, const ShaderRecompiler::ShaderVertexInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target) {
    using namespace ShaderRecompiler;
    if (state.stage != Graphics::Container::NativeHardwareStage::Vertex || input.logicalStage != IrShaderStage::Vertex) throw std::invalid_argument("Native vertex translation received a different hardware stage");
    if (target.subgroupSize != 32 && target.subgroupSize != 64) throw std::invalid_argument("Native vertex target subgroup size is invalid");
    if (input.hostSubgroupSize != target.subgroupSize) throw std::invalid_argument("Native vertex input and target subgroup sizes do not match");
    if (state.floatControl > 255) throw std::invalid_argument("Native vertex float mode is invalid");
    if (target.float32FlushToZero && (target.float32DenormPreserve || (state.floatControl & 0x30) != 0)) throw std::invalid_argument("Native vertex target float controls disagree with the initial mode");
    ShaderFloatMode mode{state.floatControl, state.dx10Clamp, state.ieeeMode, false};
    auto options = MakeNativeOptions(ShaderStageKind::Vertex, frontend.GetWaveSize(), userRegisters, mode, target);
    options.gcnLegacyVertexInputs = true;
    options.inputInfo.vertex = &input;
    auto result = TranslateNativeProgram(frontend.GetDecoded(), frontend.GetGraph(), options);
    SsaBuilder{}.Rewrite(result);
    OptimizeNativeProgram(result, frontend.GetWaveSize());
    MarkNativeBufferDescriptors(result);
    return result;
}

AnyBox::Shader::StageCompilation AnyBox::Shader::CompileNativeVertex(std::span<const std::byte> program, const ShaderRecompiler::ShaderVertexInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    auto state = Graphics::Container::ReadNativeStageState(program);
    NativeFrontend frontend(program);
    auto lowered = LowerNativeVertex(frontend, state, input, userRegisters, target);
    return CompileVertex(lowered, input, target, layout);
}
