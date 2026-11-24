#include "Plan.hpp"

std::optional<AnyBox::Shader::Reductions::Plan> AnyBox::Shader::Reductions::FindInteger(const Graph& graph, std::uint32_t result) {
    const auto& root = graph.Get(result);
    if (!root.Is(spv::OpIAdd, 5) || !graph.UnsignedWord(root.words[1]) || root.block == 0) return std::nullopt;
    std::uint32_t lanes = 0;
    std::uint32_t source = 0;
    std::uint32_t active = 0;
    std::uint32_t scope = 0;
    std::optional<bool> prefix;
    auto current = result;
    for (unsigned index = 0; index < 32; ++index) {
        const auto& sum = graph.Get(current);
        auto termId = current;
        if (index != 31) {
            if (!sum.Is(spv::OpIAdd, 5) || sum.block != root.block || sum.words[1] != root.words[1]) return std::nullopt;
            termId = sum.words[4];
            current = sum.words[3];
        }
        const auto& term = graph.Get(termId);
        if (!term.Is(spv::OpSelect, 6) || term.block != root.block || term.words[1] != root.words[1] || !graph.Integer(term.words[5], 0)) return std::nullopt;
        const auto& shuffle = graph.Get(term.words[4]);
        if (!shuffle.Is(spv::OpGroupNonUniformShuffle, 6) || shuffle.block != root.block || shuffle.words[1] != root.words[1] || !graph.Integer(shuffle.words[3], spv::ScopeSubgroup)) return std::nullopt;
        unsigned lane = 0;
        std::uint32_t word = 0;
        if (!graph.Selector(shuffle.words[5], root.block, lane, word) || (lanes & (std::uint32_t{1} << lane)) != 0) return std::nullopt;
        const auto* selector = &graph.Get(shuffle.words[5]);
        if (selector->Is(spv::OpBitwiseAnd, 5)) selector = &graph.Get(selector->words[3]);
        auto present = selector->words[3];
        auto isPrefix = term.words[3] != present;
        if (isPrefix) {
            const auto& condition = graph.Get(term.words[3]);
            if (!condition.Is(spv::OpLogicalAnd, 5) || condition.block != root.block || condition.words[3] != present) return std::nullopt;
            const auto& before = graph.Get(condition.words[4]);
            if (!before.Is(spv::OpULessThan, 5) || before.block != root.block || !graph.Integer(before.words[3], lane) || !graph.LaneIndex(before.words[4], root.block)) return std::nullopt;
        }
        if (prefix && *prefix != isPrefix) return std::nullopt;
        prefix = isPrefix;
        if (index != 0 && (source != shuffle.words[4] || active != word)) return std::nullopt;
        source = shuffle.words[4];
        active = word;
        scope = shuffle.words[3];
        lanes |= std::uint32_t{1} << lane;
    }
    if (lanes != UINT32_MAX) return std::nullopt;
    auto group = *prefix ? spv::GroupOperationExclusiveScan : spv::GroupOperationReduce;
    return Plan{spv::OpGroupNonUniformIAdd, scope, source, static_cast<std::uint32_t>(group)};
}
