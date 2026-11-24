#include "Fixtures.hpp"
#include "../../src/backend/reductions/Reduction.hpp"
#include <spirv/unified1/spirv.hpp>

void CheckWaveReductionProof(std::span<const std::uint32_t> code) {
    std::vector<std::size_t> shuffles;
    for (std::size_t offset = 5; offset < code.size(); offset += code[offset] >> 16) {
        if ((code[offset] & 65535) == spv::OpGroupNonUniformShuffle) shuffles.push_back(offset);
    }
    Fixtures::Require(shuffles.size() == 32);
    std::vector<std::uint32_t> original(code.begin(), code.end());
    Fixtures::Require(AnyBox::Shader::ReduceWaveOperations(original) == 1);
    for (auto offset : shuffles) {
        std::vector<std::uint32_t> changed(code.begin(), code.end());
        changed[offset + 5] = changed[offset + 3];
        auto before = changed;
        Fixtures::Require(AnyBox::Shader::ReduceWaveOperations(changed) == 0 && changed == before);
        changed.assign(code.begin(), code.end());
        changed[offset + 4] = changed[offset + 3];
        Fixtures::Require(AnyBox::Shader::ReduceWaveOperations(changed) == 0);
    }
    for (std::size_t index = 0; index < shuffles.size(); ++index) {
        std::vector<std::uint32_t> changed(code.begin(), code.end());
        changed[shuffles[index] + 5] = changed[shuffles[(index + 1) % shuffles.size()] + 5];
        Fixtures::Require(AnyBox::Shader::ReduceWaveOperations(changed) == 0);
    }
}
