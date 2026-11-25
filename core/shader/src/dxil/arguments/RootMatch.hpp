#pragma once

#include <anybox/shader/RootArguments.hpp>
#include <optional>

namespace AnyBox::Shader::Dxil {
std::optional<RootArgumentRange> MatchRootArgument(const BufferBinding& binding, const Graphics::Container::RootParameter& parameter, std::uint32_t index, std::uint32_t visibility);
}
