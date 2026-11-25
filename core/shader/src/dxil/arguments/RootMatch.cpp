#include "RootMatch.hpp"
#include <stdexcept>

using namespace AnyBox::Shader;
using namespace AnyBox::Graphics::Container;

std::optional<RootArgumentRange> AnyBox::Shader::Dxil::MatchRootArgument(const BufferBinding& binding, const RootParameter& parameter, std::uint32_t index, std::uint32_t visibility) {
    if (static_cast<unsigned>(parameter.kind) > 4 || parameter.visibility > 7) throw std::invalid_argument("Root parameter type or visibility is invalid");
    if ((parameter.flags & ~0xEU) != 0) throw std::invalid_argument("Root descriptor flags are invalid");
    if (parameter.visibility != 0 && parameter.visibility != visibility) return std::nullopt;
    auto match = [&](RootArgumentKind kind, std::uint32_t offset, std::uint32_t words) { return RootArgumentRange{binding, kind, index, offset, 0, words}; };
    if (parameter.kind != RootParameterKind::Table) {
        auto category = parameter.kind == RootParameterKind::Constants || parameter.kind == RootParameterKind::ConstantBuffer ? ResourceClass::ConstantBuffer : parameter.kind == RootParameterKind::ShaderResource ? ResourceClass::ShaderResource : ResourceClass::UnorderedAccess;
        if (parameter.space != binding.space || parameter.shaderRegister != binding.firstRegister || category != binding.resourceClass) return std::nullopt;
        if (binding.count != 1) throw std::runtime_error("Root descriptors and constants cannot bind resource arrays");
        if (parameter.kind == RootParameterKind::Constants) {
            if (parameter.wordCount == 0 || binding.byteSize > static_cast<std::uint64_t>(parameter.wordCount) * 4) throw std::invalid_argument("Root constants do not cover the declared constant buffer");
            return match(RootArgumentKind::Constants, 0, parameter.wordCount);
        }
        return match(RootArgumentKind::Buffer, 0, BufferArgumentPlan::WordsPerBuffer);
    }
    std::optional<RootArgumentRange> result;
    std::uint64_t cursor = 0;
    auto end = static_cast<std::uint64_t>(binding.firstRegister) + binding.count;
    for (const auto& range : parameter.ranges) {
        if (static_cast<unsigned>(range.kind) > 3 || (range.flags & ~0x1000FU) != 0) throw std::invalid_argument("Root descriptor range kind or flags are invalid");
        if (range.count == 0) throw std::invalid_argument("Root descriptor range is empty");
        std::uint64_t offset = range.tableOffset == UINT32_MAX ? cursor : range.tableOffset;
        if (offset > UINT32_MAX) throw std::invalid_argument("Root descriptor append follows an unbounded or overflowing range");
        auto limit = range.count == UINT32_MAX ? static_cast<std::uint64_t>(UINT32_MAX) + 1 : static_cast<std::uint64_t>(range.firstRegister) + range.count;
        if (limit > static_cast<std::uint64_t>(UINT32_MAX) + 1) throw std::invalid_argument("Root descriptor register range overflows");
        cursor = range.count == UINT32_MAX ? static_cast<std::uint64_t>(UINT32_MAX) + 1 : offset + range.count;
        if (cursor > static_cast<std::uint64_t>(UINT32_MAX) + 1) throw std::invalid_argument("Root descriptor table range overflows");
        if (static_cast<unsigned>(range.kind) != static_cast<unsigned>(binding.resourceClass) || range.space != binding.space || binding.firstRegister < range.firstRegister || end > limit) continue;
        auto selected = offset + binding.firstRegister - range.firstRegister;
        if (selected + binding.count > static_cast<std::uint64_t>(UINT32_MAX) + 1) throw std::invalid_argument("Root descriptor table selection overflows");
        if (result) throw std::invalid_argument("Shader buffer has overlapping root descriptor ranges");
        result = match(RootArgumentKind::Table, static_cast<std::uint32_t>(selected), binding.count * BufferArgumentPlan::WordsPerBuffer);
    }
    return result;
}
