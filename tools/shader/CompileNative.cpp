#include "Compilation.hpp"
#include "AuditTarget.hpp"
#include "DxilModule.hpp"
#include "ComputeEntry.hpp"
#include "ResourceLayout.hpp"
#include <anybox/shader/NativeArguments.hpp>
#include <anybox/graphics/container/Native.hpp>
#include <Recompiler.hpp>
#include <algorithm>

AnyBox::Shader::ComputeArtifact AnyBox::Shader::Tools::CompileNative(std::span<const std::byte> code) {
    DxilModule module(code);
    auto entry = Dxil::ReadComputeEntry(module.GetModule());
    Dxil::ResourceLayout resources(module.GetModule());
    Graphics::Container::RootSignatureLayout root{};
    root.version = 2;
    if (!resources.GetBindings().empty()) root = Graphics::Container::ReadRootSignatureLayout(code);
    RootArgumentPlan arguments(resources.GetBindings(), root);
    auto programs = Graphics::Container::UnpackNativePrograms(code);
    if (programs.size() != 1) throw std::runtime_error("Native compute pipeline requires exactly one hardware program");
    auto state = Graphics::Container::ReadNativeComputeState(programs.front());
    if (state.threads != entry.info.threads) throw std::invalid_argument("Native and DXIL compute workgroup dimensions disagree");
    auto bindings = Graphics::Container::ReadNativeComputeBindings(programs.front());
    NativeFrontend frontend(programs.front());
    auto target = AuditTarget();
    ShaderRecompiler::ShaderFloatMode mode{state.floatControl, state.dx10Clamp, state.ieeeMode, false};
    auto translated = LowerNativeCompute(frontend, state, mode, target);
    BindNativeRootArguments(translated, bindings.roots, arguments);
    auto compiled = CompileCompute(translated.program, translated.input, target, {0, 0, 0, 128});
    auto artifact = MakeArtifact(std::move(compiled), resources.GetBindings(), translated.input, HashShader(code));
    ValidateComputeArtifact(artifact);
    return artifact;
}
