#include <anybox/shader/Requirements.hpp>
#include <anybox/graphics/container/Dxil.hpp>
#include <anybox/graphics/container/Native.hpp>
#include <stdexcept>
#include <string>

std::optional<std::uint32_t> AnyBox::Shader::GetComputeWaveSize(std::span<const std::byte> code) {
    using namespace Graphics::Container;
    DxbcView container(code);
    auto part = container.FindPart(0x4C495844);
    auto modern = container.FindPart(0x58454853);
    auto legacy = container.FindPart(0x52444853);
    auto programs = static_cast<unsigned>(part.has_value()) + static_cast<unsigned>(modern.has_value()) + static_cast<unsigned>(legacy.has_value());
    if (programs != 1) throw std::invalid_argument("Compute shader container must contain one executable shader part");
    if (!part) return std::nullopt;
    DxilView program(part->data);
    if ((program.GetProgramVersion() >> 16) != 5) throw std::invalid_argument("Compute pipeline shader has a different shader stage");
    if (!program.HasEmbeddedData()) return std::nullopt;
    auto features = container.FindPart(0x30494653);
    if (!features || features->data.size() != 8) throw std::invalid_argument("Xbox compute shader feature requirements are missing or invalid");
    auto usesWaveOperations = (std::to_integer<unsigned>(features->data[1]) & 0x40) != 0;
    auto options = program.FindEmbeddedPart(10);
    std::optional<std::uint32_t> waveSize;
    bool sourceSpecified = false;

    if (options) {
        CompilerOptionsView values(options->data);
        for (std::uint32_t index = 0; index < values.GetCount(); ++index) {
            auto option = values.GetOption(index);
            if (option.key != 627 && option.key != 677) throw std::runtime_error("Xbox shader compiler option is not implemented: " + std::to_string(option.key));
            if (option.value != "0" && option.value != "1") throw std::invalid_argument("Xbox shader boolean compiler option is invalid");
            if (option.key == 677) {
                if (sourceSpecified) throw std::invalid_argument("Xbox shader inline source option is duplicated");
                sourceSpecified = true;
                continue;
            }
            if (waveSize) throw std::invalid_argument("Xbox shader wave size option is duplicated");
            waveSize = option.value == "1" ? 32U : 64U;
        }
    }
    if (!usesWaveOperations) return std::nullopt;
    if (!waveSize) {
        for (std::uint32_t index = 0; index < program.GetEmbeddedPartCount(); ++index) {
            auto embedded = program.GetEmbeddedPart(index);
            if (embedded.tag != 13) continue;
            auto native = UnpackXbch(embedded.data);
            auto size = FssmView(native).GetWaveSize();
            if (waveSize && *waveSize != size) throw std::runtime_error("Xbox compute shader native variants require different wave sizes");
            waveSize = size;
        }
    }
    if (!waveSize) throw std::runtime_error("Xbox compute shader wave size is unspecified");
    return waveSize;
}
