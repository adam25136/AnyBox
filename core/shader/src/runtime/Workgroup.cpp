#include "Requirements.hpp"
#include <spirv/unified1/spirv.hpp>
#include <stdexcept>

std::array<std::uint32_t, 3> AnyBox::Shader::Runtime::ReadComputeWorkgroup(const ComputeArtifact& artifact) {
    const auto& code = artifact.code;
    std::uint32_t entry = 0;
    for (std::size_t offset = 5; offset < code.size(); offset += code[offset] >> 16) {
        if ((code[offset] & 65535) != spv::OpEntryPoint || (code[offset] >> 16) < 5) continue;
        if (code[offset + 1] != spv::ExecutionModelGLCompute || code[offset + 3] != 0x6E69616D || code[offset + 4] != 0) continue;
        if (entry != 0 || code[offset + 2] == 0) throw std::invalid_argument("Offline compute pipeline entry point is invalid or duplicated");
        entry = code[offset + 2];
    }
    if (entry == 0) throw std::invalid_argument("Offline compute pipeline has no main compute entry point");
    bool found = false;
    std::array<std::uint32_t, 3> size{};
    for (std::size_t offset = 5; offset < code.size(); offset += code[offset] >> 16) {
        if ((code[offset] & 65535) != spv::OpExecutionMode || (code[offset] >> 16) < 3 || code[offset + 1] != entry || code[offset + 2] != spv::ExecutionModeLocalSize) continue;
        if (found || (code[offset] >> 16) != 6) throw std::invalid_argument("Offline compute pipeline local size is invalid or duplicated");
        found = true;
        size = {code[offset + 3], code[offset + 4], code[offset + 5]};
    }
    if (!found) throw std::runtime_error("Offline compute pipeline requires a literal local size");
    auto expected = artifact.threads;
    if (artifact.waveSize == 64 && artifact.hostSubgroupSize == 32 && (artifact.executionFlags & (NativeComputeInvocations | SpannedComputeWave)) == 0) {
        auto invocations = expected[0] * expected[1] * expected[2];
        expected = {(invocations + 63) / 64 * 32, 1, 1};
    }
    if (size != expected) throw std::invalid_argument("Offline compute pipeline local size does not match its wave layout");
    return size;
}
