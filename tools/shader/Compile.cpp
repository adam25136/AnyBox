#include "Compilation.hpp"
#include "AuditTarget.hpp"
#include "DxilModule.hpp"
#include "ComputeEntry.hpp"
#include "ComputeCompiler.hpp"
#include "lowering/Function.hpp"
#include "arguments/RootBindings.hpp"
#include <anybox/shader/Requirements.hpp>
#include <algorithm>

AnyBox::Shader::ComputeArtifact AnyBox::Shader::Tools::CompileDxil(std::span<const std::byte> code) {
    using namespace ShaderRecompiler;
    DxilModule module(code);
    auto entry = Dxil::ReadComputeEntry(module.GetModule());
    Dxil::ResourceLayout resources(module.GetModule());
    Graphics::Container::RootSignatureLayout root{};
    root.version = 2;
    if (!resources.GetBindings().empty()) root = Graphics::Container::ReadRootSignatureLayout(code);
    RootArgumentPlan arguments(resources.GetBindings(), root);
    Dxil::RootBindings bindings(arguments);
    IrProgram program;
    program.Resources().userDataCount = arguments.GetUserRegisterCount();
    ShaderComputeInputInfo input;
    std::copy(entry.info.threads.begin(), entry.info.threads.end(), input.threadsNum);
    auto wave = GetComputeWaveSize(code);
    if (wave && entry.info.preferredWaveSize && *wave != entry.info.preferredWaveSize) throw std::runtime_error("Xbox and DXIL wave size requirements disagree");
    input.waveSize = wave.value_or(entry.info.preferredWaveSize ? entry.info.preferredWaveSize : 32);
    input.hostSubgroupSize = 32;
    if (input.waveSize != input.hostSubgroupSize) throw std::runtime_error("Offline compute wave size adaptation is not implemented");
    program.SetWaveSize(input.waveSize);
    Dxil::FunctionLowering(program, resources, bindings).Run(*entry.function);
    input.ldsSizeDwords = program.Info().sharedMemoryBytes / 4;
    auto result = Dxil::CompileCompute(program, input, AuditTarget(), {0, 0, 0, 128});
    return MakeArtifact(std::move(result), resources.GetBindings(), input, HashShader(code));
}
