#include "RootMatch.hpp"
#include <algorithm>
#include <stdexcept>

using namespace AnyBox::Shader;

RootArgumentPlan::RootArgumentPlan(std::span<const BufferBinding> bindings, const Graphics::Container::RootSignatureLayout& root, std::uint32_t firstUserRegister, std::uint32_t visibility) : userRegisterCount(firstUserRegister) {
    static_cast<void>(BufferArgumentPlan(bindings));
    if (root.version != 1 && root.version != 2) throw std::invalid_argument("Root argument signature version is invalid");
    if (visibility > 7) throw std::invalid_argument("Root argument shader visibility is invalid");
    if ((root.flags & ~0x83000FFFU) != 0) throw std::invalid_argument("Root argument signature flags are invalid");
    if (firstUserRegister > BufferArgumentPlan::UserRegisterCapacity) throw std::invalid_argument("Root argument register base exceeds the ABI capacity");
    if ((root.flags & 0x80) != 0) throw std::invalid_argument("A local root signature cannot bind this pipeline");
    if (root.parameters.size() > UINT32_MAX) throw std::length_error("Root signature contains too many parameters");
    for (const auto& binding : bindings) {
        std::optional<RootArgumentRange> mapped;
        for (std::size_t index = 0; index < root.parameters.size(); ++index) {
            auto range = Dxil::MatchRootArgument(binding, root.parameters[index], static_cast<std::uint32_t>(index), visibility);
            if (!range) continue;
            if (mapped) throw std::invalid_argument("Shader buffer matches more than one root parameter");
            mapped = *range;
        }
        if (!mapped) throw std::runtime_error("Shader buffer is not covered by the pipeline root signature");
        if (mapped->wordCount > BufferArgumentPlan::UserRegisterCapacity - userRegisterCount) throw std::length_error("Root arguments exceed the ABI register capacity");
        mapped->firstUserRegister = userRegisterCount;
        userRegisterCount += mapped->wordCount;
        ranges.push_back(std::move(*mapped));
    }
}

std::span<const RootArgumentRange> RootArgumentPlan::GetRanges() const { return ranges; }

std::uint32_t RootArgumentPlan::GetUserRegisterCount() const { return userRegisterCount; }

const RootArgumentRange& RootArgumentPlan::Find(const BufferBinding& binding) const {
    auto found = std::ranges::find_if(ranges, [&](const auto& range) { return range.binding == binding; });
    if (found == ranges.end()) throw std::invalid_argument("Shader buffer is absent from the root argument plan");
    return *found;
}
