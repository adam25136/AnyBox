#include "Local.hpp"
#include <algorithm>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::LocalLowering;

IrValue& LocalLowering::constantWord(const Pointer& pointer, const Access& access) {
    const auto& words = *pointer.allocation.constants;
    if (words.empty()) throw std::logic_error("DXIL global constant allocation is empty");
    if (pointer.constant) return builder.Constant(words.at(static_cast<std::size_t>(*pointer.constant) / 4));
    auto* result = &builder.Constant(words.front());
    if (!std::ranges::all_of(words, [&](auto word) { return word == words.front(); })) {
        result = &builder.Constant(0);
        auto& index = builder.ShiftRightLogical(builder.CompositeExtract(*pointer.offset, 0), builder.Constant(2));
        for (std::size_t word = 0; word < words.size(); ++word) {
            if (words[word] == 0) continue;
            auto& selected = builder.IEqual(index, builder.Constant(static_cast<std::uint32_t>(word)));
            result = &builder.Select(selected, builder.Constant(words[word]), *result);
        }
    }
    return builder.Select(access.active, *result, builder.Constant(0));
}
