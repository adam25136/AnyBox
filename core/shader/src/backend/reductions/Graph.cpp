#include "Graph.hpp"
#include <cstring>
#include <stdexcept>

using namespace AnyBox::Shader::Reductions;

Graph::Graph(std::span<const std::uint32_t> code) {
    if (code.size() < 5 || code[0] != 0x07230203 || code[3] > code.size() * 4) throw std::invalid_argument("Wave reduction received invalid SPIR-V framing");
    definitions.resize(code[3]);
    glslImports.resize(code[3]);
    laneVariables.resize(code[3]);
    std::uint32_t block = 0;
    for (std::size_t offset = 5; offset < code.size();) {
        auto count = code[offset] >> 16;
        if (count == 0 || count > code.size() - offset) throw std::invalid_argument("Wave reduction received a truncated instruction");
        auto words = code.subspan(offset, count);
        auto opcode = static_cast<spv::Op>(words[0] & 65535);
        if (opcode == spv::OpDecorate && count == 4 && words[2] == spv::DecorationBuiltIn && words[3] == spv::BuiltInSubgroupLocalInvocationId) {
            if (words[1] >= laneVariables.size()) throw std::invalid_argument("Wave reduction builtin identifier is invalid");
            laneVariables[words[1]] = true;
        }
        if (opcode == spv::OpFunction || opcode == spv::OpFunctionEnd) block = 0;
        if (opcode == spv::OpLabel && count == 2) block = words[1];
        bool result = false;
        bool typed = false;
        spv::HasResultAndType(opcode, &result, &typed);
        if (result) {
            auto position = typed ? 2u : 1u;
            if (count <= position || words[position] >= definitions.size()) throw std::invalid_argument("Wave reduction received an invalid result identifier");
            definitions[words[position]] = {words, block};
        }
        if (opcode == spv::OpExtInstImport && count == 6 && std::memcmp(words.data() + 2, "GLSL.std.450", 13) == 0) glslImports[words[1]] = true;
        offset += count;
    }
}

const Node& Graph::Get(std::uint32_t id) const {
    static const Node empty;
    return id < definitions.size() ? definitions[id] : empty;
}

bool Graph::LaneIndex(std::uint32_t id, std::uint32_t block) const {
    const auto& value = Get(id);
    return value.Is(spv::OpLoad, 4) && value.block == block && UnsignedWord(value.words[1]) && value.words[3] < laneVariables.size() && laneVariables[value.words[3]];
}

bool Graph::UnsignedWord(std::uint32_t id) const {
    const auto& type = Get(id);
    return type.Is(spv::OpTypeInt, 4) && type.words[2] == 32 && type.words[3] == 0;
}

bool Graph::Integer(std::uint32_t id, std::uint32_t value) const {
    const auto& node = Get(id);
    return node.Is(spv::OpConstant, 4) && UnsignedWord(node.words[1]) && node.words[3] == value;
}

bool Graph::Glsl(const Node& node, GLSLstd450 operation, std::size_t operands) const {
    return node.Is(spv::OpExtInst, operands + 5) && node.words[3] < glslImports.size() && glslImports[node.words[3]] && node.words[4] == static_cast<unsigned>(operation);
}

bool Graph::ActiveWord(std::uint32_t id, std::uint32_t block) const {
    const auto& word = Get(id);
    if (!word.Is(spv::OpCompositeExtract, 5) || word.words[4] != 0 || word.block != block) return false;
    const auto& ballot = Get(word.words[3]);
    if (!ballot.Is(spv::OpGroupNonUniformBallot, 5) || ballot.block != block || !Integer(ballot.words[3], spv::ScopeSubgroup)) return false;
    return Get(ballot.words[4]).Is(spv::OpConstantTrue, 3);
}

bool Graph::MinMax(const Node& node, bool maximum, std::uint32_t& left, std::uint32_t& right) const {
    if (Glsl(node, maximum ? GLSLstd450UMax : GLSLstd450UMin, 2)) {
        left = node.words[5];
        right = node.words[6];
        return true;
    }
    if (!node.Is(spv::OpSelect, 6)) return false;
    const auto& comparison = Get(node.words[3]);
    if (!comparison.Is(maximum ? spv::OpUGreaterThan : spv::OpULessThan, 5) || comparison.block != node.block) return false;
    left = node.words[4];
    right = node.words[5];
    return comparison.words[3] == left && comparison.words[4] == right;
}
