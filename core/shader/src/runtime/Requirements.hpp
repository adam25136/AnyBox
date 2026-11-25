#pragma once

#include <anybox/shader/Artifact.hpp>
#include <anybox/vulkan/Context.hpp>

namespace AnyBox::Shader::Runtime {
void CheckRequirements(const Vulkan::Context& context, const ComputeArtifact& artifact);
std::array<std::uint32_t, 3> ReadComputeWorkgroup(const ComputeArtifact& artifact);
}
