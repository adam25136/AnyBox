#include <anybox/shader/RootArgumentWriter.hpp>

bool AnyBox::Shader::RootArgumentWriter::RequiresVolatileTables(const Graphics::Container::RootSignatureLayout& root) const {
    for (const auto& step : steps) {
        if (step.range.kind != RootArgumentKind::Table) continue;
        if (root.version != 2) return true;
        const auto& binding = step.range.binding;
        auto selected = static_cast<std::uint64_t>(binding.firstRegister) + step.element;
        for (const auto& range : root.parameters.at(step.range.parameter).ranges) {
            if (static_cast<unsigned>(range.kind) != static_cast<unsigned>(binding.resourceClass) || range.space != binding.space || selected < range.firstRegister) continue;
            if (range.count != UINT32_MAX && selected - range.firstRegister >= range.count) continue;
            if ((range.flags & 1) != 0) return true;
        }
    }
    return false;
}
