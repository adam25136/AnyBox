#pragma once

#include <cstdint>

namespace AnyBox::Shader::Dxil {
enum class LaneReadMode : std::uint64_t { Xbox, Active };
}
