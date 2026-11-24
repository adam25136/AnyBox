#pragma once

#define SPV_ENABLE_UTILITY_CODE
#include <spirv/unified1/spirv.hpp>
#include <spirv/unified1/GLSL.std.450.h>
#include <span>
#include <vector>
#include <cstdint>

namespace AnyBox::Shader::Reductions {
struct Node {
    std::span<const std::uint32_t> words;
    std::uint32_t block = 0;
    bool Is(spv::Op opcode, std::size_t size) const { return words.size() == size && (words[0] & 65535) == static_cast<unsigned>(opcode); }
};

class Graph {
public:
    explicit Graph(std::span<const std::uint32_t> code);
    const Node& Get(std::uint32_t id) const;
    bool Integer(std::uint32_t id, std::uint32_t value) const;
    bool UnsignedWord(std::uint32_t id) const;
    bool Glsl(const Node& node, GLSLstd450 operation, std::size_t operands) const;
    bool MinMax(const Node& node, bool maximum, std::uint32_t& left, std::uint32_t& right) const;
    bool ActiveWord(std::uint32_t id, std::uint32_t block) const;
    bool LaneIndex(std::uint32_t id, std::uint32_t block) const;
    bool Selector(std::uint32_t id, std::uint32_t block, unsigned& lane, std::uint32_t& active) const;

private:
    std::vector<Node> definitions;
    std::vector<bool> glslImports;
    std::vector<bool> laneVariables;
};
}
