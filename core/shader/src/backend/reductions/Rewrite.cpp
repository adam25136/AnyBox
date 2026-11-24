#include "Plan.hpp"
#include "Reduction.hpp"
#include <array>
#include <unordered_map>

std::size_t AnyBox::Shader::ReduceWaveOperations(std::vector<std::uint32_t>& code) {
    Reductions::Graph graph(code);
    std::unordered_map<std::size_t, Reductions::Plan> replacements;
    bool capability = false;
    for (std::size_t offset = 5; offset < code.size(); offset += code[offset] >> 16) {
        auto opcode = static_cast<spv::Op>(code[offset] & 65535);
        if (opcode == spv::OpCapability && code[offset + 1] == spv::CapabilityGroupNonUniformArithmetic) capability = true;
        if (opcode != spv::OpExtInst && opcode != spv::OpSelect && opcode != spv::OpIAdd) continue;
        auto plan = opcode == spv::OpIAdd ? Reductions::FindInteger(graph, code[offset + 2]) : Reductions::Find(graph, code[offset + 2]);
        if (plan) replacements.emplace(offset, *plan);
    }
    if (replacements.empty()) return 0;
    std::vector<std::uint32_t> output(code.begin(), code.begin() + 5);
    if (!capability) output.insert(output.end(), {(2u << 16) | spv::OpCapability, spv::CapabilityGroupNonUniformArithmetic});
    for (std::size_t offset = 5; offset < code.size();) {
        auto count = code[offset] >> 16;
        if (auto found = replacements.find(offset); found != replacements.end()) {
            const auto& plan = found->second;
            std::array<std::uint32_t, 6> instruction{(6u << 16) | plan.operation, code[offset + 1], code[offset + 2], plan.scope, plan.group, plan.source};
            output.insert(output.end(), instruction.begin(), instruction.end());
        } else {
            output.insert(output.end(), code.begin() + offset, code.begin() + offset + count);
        }
        offset += count;
    }
    code = std::move(output);
    return replacements.size();
}
