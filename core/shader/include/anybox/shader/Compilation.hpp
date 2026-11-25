#pragma once

#include <SpirvBackend/SpirvEmitter.hpp>

namespace AnyBox::Shader {
struct StageCompilation {
    std::vector<std::uint32_t> code;
    ShaderRecompiler::CompiledBindingLayout bindings;
    ShaderRecompiler::SpirvEmitInfo info;
};
}
