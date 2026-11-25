#pragma once

#include <Optimization/ShaderStageInputInfo.hpp>
#include <SpirvBackend/SpirvEmitter.hpp>

namespace AnyBox::Shader {
inline bool UsesSpannedWave(const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target) {
    if (!target.spannedWaves || input.waveSize != 64 || input.hostSubgroupSize != 32) return false;
    std::uint64_t threads = 1;
    for (auto size : input.threadsNum) {
        if (size == 0 || size > 1024) return false;
        threads *= size;
    }
    return threads == 64;
}
}
