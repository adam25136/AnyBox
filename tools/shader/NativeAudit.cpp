#include "NativeAudit.hpp"
#include "AuditTarget.hpp"
#include <anybox/graphics/container/Native.hpp>
#include <anybox/graphics/container/Dxil.hpp>
#include <anybox/graphics/container/NativeBindings.hpp>
#include <anybox/shader/NativeCompute.hpp>
#include <anybox/shader/Native.hpp>
#include <anybox/shader/ComputeCompiler.hpp>
#include <Recompiler.hpp>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {
std::string Decode(std::span<const std::byte> program) {
    AnyBox::Shader::NativeFrontend frontend(program);
    return "instructions=" + std::to_string(frontend.GetDecoded().instructions.size()) + " blocks=" + std::to_string(frontend.GetGraph().blocks.size());
}

void DescribeFlow(std::ostream& output, const ShaderRecompiler::IrProgram& program) {
    const auto& order = program.BlockOrder();
    std::unordered_map<const ShaderRecompiler::IrBlock*, std::size_t> positions;
    for (std::size_t index = 0; index < order.size(); ++index) positions.emplace(order[index], index);
    output << "wave=" << program.WaveSize() << '\n';
    for (std::size_t index = 0; index < order.size(); ++index) {
        output << "cfg $" << index << " loopHeader=" << program.Metadata().blockInfo.at(index).terminator.loopHeader << " predecessors=";
        for (auto* block : order[index]->Predecessors()) output << '$' << positions.at(block) << ',';
        output << " successors=";
        for (auto* block : order[index]->Successors()) output << '$' << positions.at(block) << ',';
        output << '\n';
    }
}
}

std::string AnyBox::Shader::Tools::InspectNative(std::span<const std::byte> container) {
    auto programs = Graphics::Container::UnpackNativePrograms(container);
    std::ostringstream output;
    for (std::size_t index = 0; index < programs.size(); ++index) {
        Graphics::Container::FssmView view(programs[index]);
        output << "program=" << index << " wave=" << view.GetWaveSize() << " codeBytes=" << view.GetCode().size() << ' ';
        try {
            output << Decode(programs[index]);
            Graphics::Container::DxbcView outer(container);
            Graphics::Container::DxilView header(outer.FindPart(0x4C495844)->data);
            if ((header.GetProgramVersion() >> 16) == 5) {
                auto bindings = Graphics::Container::ReadNativeComputeBindings(programs[index]);
                output << " roots=" << bindings.roots.size() << " resources=" << bindings.resources.size();
                for (const auto& root : bindings.roots) output << " root[" << static_cast<unsigned>(root.rootParameter) << "]={kind=" << static_cast<unsigned>(root.kind) << ",register=" << static_cast<unsigned>(root.firstUserRegister) << ",words=" << root.registerCount << '}';
            }
        }
        catch (const std::exception& error) { output << "decodeError=" << error.what(); }
        output << '\n';
    }
    return output.str();
}

std::size_t AnyBox::Shader::Tools::AuditNative(std::span<const std::byte> container) {
    auto programs = Graphics::Container::UnpackNativePrograms(container);
    Graphics::Container::DxbcView outer(container);
    Graphics::Container::DxilView header(outer.FindPart(0x4C495844)->data);
    bool compute = (header.GetProgramVersion() >> 16) == 5;
    std::size_t words = 0;
    for (std::size_t index = 0; index < programs.size(); ++index) {
        try {
            if (compute) {
                auto state = Graphics::Container::ReadNativeComputeState(programs[index]);
                NativeFrontend frontend(programs[index]);
                auto target = AuditTarget();
                auto compiled = CompileNativeCompute(frontend, state, target, {0, 0, 0, 128});
                if (compiled.code.size() < 5 || compiled.code.front() != 0x07230203) throw std::runtime_error("Native compute compiler returned invalid SPIR-V framing");
                words += compiled.code.size();
            } else (void)Decode(programs[index]);
        }
        catch (const std::exception& error) { throw std::runtime_error("Native program " + std::to_string(index) + ": " + error.what()); }
    }
    if (compute) return words;
    throw std::runtime_error("Xbox native stage parameters frontend is not implemented");
}

std::string AnyBox::Shader::Tools::InspectNativeIr(std::span<const std::byte> container) {
    auto programs = Graphics::Container::UnpackNativePrograms(container);
    std::ostringstream output;
    for (std::size_t index = 0; index < programs.size(); ++index) {
        auto state = Graphics::Container::ReadNativeComputeState(programs[index]);
        NativeFrontend frontend(programs[index]);
        ShaderRecompiler::ShaderFloatMode mode{state.floatControl, state.dx10Clamp, state.ieeeMode, false};
        auto lowered = LowerNativeCompute(frontend, state, mode, AuditTarget());
        output << "program=" << index << " floatMode=" << state.floatControl << " dx10Clamp=" << state.dx10Clamp << " ieeeMode=" << state.ieeeMode << " fp16Overflow=0\n";
        DescribeFlow(output, lowered.program);
        output << ShaderRecompiler::ProgramToString(lowered.program);
    }
    return output.str();
}
