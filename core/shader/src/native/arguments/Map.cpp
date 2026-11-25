#include "Map.hpp"
#include <algorithm>
#include <stdexcept>

AnyBox::Shader::NativeArgumentMap AnyBox::Shader::MapNativeArguments(std::span<const Graphics::Container::NativeRootBinding> roots, const RootArgumentPlan& plan) {
    using Graphics::Container::NativeRootKind;
    NativeArgumentMap result;
    result.fill(UINT32_MAX);
    std::array<bool, 256> occupied{};
    for (const auto& root : roots) {
        if (root.kind != NativeRootKind::Constants && root.kind != NativeRootKind::Descriptor) throw std::runtime_error("Native root argument tables and heap bindings are not implemented");
        if (root.registerCount == 0 || root.registerCount > result.size() - root.firstUserRegister) throw std::invalid_argument("Native root argument register range is invalid");
        if (root.tableOffset != 0 || (root.kind == NativeRootKind::Descriptor && root.registerCount != 2)) throw std::invalid_argument("Native direct root argument layout is invalid");
        const RootArgumentRange* mapped = nullptr;
        for (const auto& range : plan.GetRanges()) {
            if (range.parameter != root.rootParameter || range.binding.firstRegister != root.shaderRegister) continue;
            if (mapped) throw std::runtime_error("Native root argument maps to multiple shader bindings");
            mapped = &range;
        }
        if (mapped) {
            bool constants = root.kind == NativeRootKind::Constants;
            if (constants ? mapped->kind != RootArgumentKind::Constants || mapped->wordCount != root.registerCount : mapped->kind != RootArgumentKind::Buffer || mapped->binding.count != 1) throw std::invalid_argument("Native and logical root argument layouts disagree");
        }
        for (std::uint32_t word = 0; word < root.registerCount; ++word) {
            auto index = root.firstUserRegister + word;
            if (occupied[index]) throw std::invalid_argument("Native root argument registers overlap");
            occupied[index] = true;
            if (mapped) result[index] = mapped->firstUserRegister + word;
        }
    }
    return result;
}
