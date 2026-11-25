#include "NativeAudit.hpp"
#include <anybox/graphics/container/Native.hpp>
#include <anybox/graphics/container/NativeStage.hpp>
#include <anybox/shader/Native.hpp>
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <cstring>
#include <iomanip>
#include <sstream>

std::string AnyBox::Shader::Tools::InspectNativeCode(std::span<const std::byte> container) {
    auto programs = Graphics::Container::UnpackNativePrograms(container);
    std::ostringstream output;
    for (std::size_t index = 0; index < programs.size(); ++index) {
        const auto& program = programs[index];
        NativeFrontend frontend(program);
        Graphics::Container::FssmView view(program);
        output << "program=" << index << " wave=" << frontend.GetWaveSize() << " code_offset=" << view.GetCodeSectionOffset() + 40 << '\n';
        for (std::size_t offset = 0xC0; offset + 4 <= view.GetCodeSectionOffset(); offset += 4) {
            std::uint32_t value;
            std::memcpy(&value, program.data() + offset, sizeof(value));
            output << "metadata[0x" << std::hex << offset << "]=0x" << value << std::dec << '\n';
        }
        auto state = Graphics::Container::ReadNativeStageState(program);
        output << "hardware_stage=" << static_cast<unsigned>(state.stage) << " float_mode=" << state.floatControl << " dx10_clamp=" << state.dx10Clamp << " ieee=" << state.ieeeMode << '\n';
        for (const auto& instruction : frontend.GetDecoded().instructions) {
            output << ShaderRecompiler::RdnaInstructionToString(instruction) << " semantic=" << static_cast<std::uint32_t>(instruction.op);
            output << " offset=" << instruction.memoryOffset << " words=" << instruction.dataDwordCount;
            if (instruction.family == ShaderRecompiler::RdnaInstructionFamily::EXP) output << " target=" << instruction.exportTarget << " mask=" << instruction.exportEnableMask << " compressed=" << instruction.exportIsCompressed << " last=" << instruction.exportIsLast;
            output << '\n';
        }
    }
    return output.str();
}
