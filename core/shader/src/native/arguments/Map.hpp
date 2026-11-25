#pragma once

#include <anybox/shader/NativeArguments.hpp>
#include <array>

namespace AnyBox::Shader {
using NativeArgumentMap = std::array<std::uint32_t, 256>;
NativeArgumentMap MapNativeArguments(std::span<const Graphics::Container::NativeRootBinding> roots, const RootArgumentPlan& plan);
}
