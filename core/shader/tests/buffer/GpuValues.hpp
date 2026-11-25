#pragma once

#include "Target.hpp"

namespace BufferTests {
std::vector<std::uint32_t> ReadGpu(AnyBox::Vulkan::Context& context, const AnyBox::Shader::ComputeCompilation& shader, std::size_t count, const std::array<std::uint32_t, 3>& groups, std::span<const std::byte> inputData = {}, std::span<const std::uint32_t> userData = {}, std::span<const std::byte> outputData = {});
}
