#pragma once

#include "Tests.hpp"
#include <anybox/shader/Artifact.hpp>

namespace AnyBox::Graphics::Tests {
struct DispatchArtifact {
    std::vector<std::byte> source;
    Shader::ComputeArtifact artifact;
};
DispatchArtifact MakeDispatchArtifact();
}
