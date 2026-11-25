#include <anybox/shader/NativeLinkage.hpp>
#include <array>
#include <algorithm>
#include <stdexcept>

void AnyBox::Shader::LinkNativePixelAttributes(ShaderRecompiler::ShaderPixelInputInfo& input, const Graphics::Container::NativePixelInputs& pixel, const Graphics::Container::NativeVertexOutputs& vertex) {
    if (pixel.count > pixel.attributes.size() || vertex.count > vertex.parameters.size()) throw std::invalid_argument("Native graphics interface count exceeds its table");
    std::array<std::uint32_t, 32> settings{};
    for (std::uint32_t attribute = 0; attribute < pixel.count; ++attribute) {
        const auto& source = pixel.attributes[attribute];
        if (source.kind != 6) throw std::runtime_error("Native pixel system attribute linkage is not implemented");
        if (source.componentMask == 0 || (source.componentMask & ~15u) != 0) throw std::invalid_argument("Native pixel attribute component mask is invalid");
        if (source.interpolation != 1 && source.interpolation != 2) throw std::runtime_error("Native pixel interpolation mode is not implemented");
        std::uint32_t match = 32;
        for (std::uint32_t parameter = 0; parameter < vertex.count; ++parameter) {
            const auto& output = vertex.parameters[parameter];
            if (output.kind != source.kind || output.registerIndex != source.registerIndex) continue;
            if (match != 32) throw std::invalid_argument("Native vertex shader exports a duplicated interface register");
            match = parameter;
        }
        if (match == 32) throw std::invalid_argument("Native pixel attribute has no matching vertex parameter");
        settings[attribute] = match | (source.interpolation == 1 ? 0x400u : 0u);
    }
    std::ranges::copy(settings, input.interpolatorSettings);
    input.inputNum = pixel.count;
}
