#include "../../src/backend/reductions/Reduction.hpp"
#include "../../src/backend/reductions/Graph.hpp"
#include "Fixtures.hpp"
#include <spirv/unified1/spirv.hpp>
#include <span>
#include <string>

void CheckIntegerWaveProof(std::span<const std::uint32_t> code, bool prefix) {
    std::vector<std::size_t> shuffles;
    std::vector<std::size_t> selects;
    std::vector<std::size_t> comparisons;
    for (std::size_t offset = 5; offset < code.size(); offset += code[offset] >> 16) {
        auto opcode = code[offset] & 65535;
        if (opcode == spv::OpGroupNonUniformShuffle) shuffles.push_back(offset);
        if (opcode == spv::OpSelect) selects.push_back(offset);
        if (opcode == spv::OpULessThan) comparisons.push_back(offset);
    }
    if (shuffles.size() != 32) throw std::runtime_error("Integer wave proof expected 32 shuffles, received " + std::to_string(shuffles.size()));
    std::vector<std::uint32_t> original(code.begin(), code.end());
    if (AnyBox::Shader::ReduceWaveOperations(original) != 1) throw std::runtime_error("Integer wave proof did not recognize the valid reduction; prefix=" + std::to_string(prefix));
    for (auto offset : shuffles) {
        std::vector<std::uint32_t> changed(code.begin(), code.end());
        changed[offset + 4] = changed[offset + 3];
        auto before = changed;
        if (AnyBox::Shader::ReduceWaveOperations(changed) != 0 || changed != before) throw std::runtime_error("Integer reduction accepted a mismatched shuffle source");
    }
    for (auto offset : selects) {
        for (auto shuffle : shuffles) {
            if (code[offset + 4] != code[shuffle + 2]) continue;
            std::vector<std::uint32_t> changed(code.begin(), code.end());
            changed[offset + 5] = changed[shuffle + 3];
            if (AnyBox::Shader::ReduceWaveOperations(changed) != 0) throw std::runtime_error("Integer reduction accepted a nonzero identity term");
        }
    }
    if (prefix) {
        Fixtures::Require(comparisons.size() >= 32);
        AnyBox::Shader::Reductions::Graph graph(code);
        for (auto offset : comparisons) {
            if (!graph.LaneIndex(code[offset + 4], graph.Get(code[offset + 2]).block)) continue;
            std::vector<std::uint32_t> changed(code.begin(), code.end());
            std::swap(changed[offset + 3], changed[offset + 4]);
            if (AnyBox::Shader::ReduceWaveOperations(changed) != 0) throw std::runtime_error("Integer scan accepted a reversed prefix comparison");
        }
    }
}
