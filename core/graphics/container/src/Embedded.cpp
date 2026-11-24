#include <anybox/graphics/container/Embedded.hpp>
#include "Integer.hpp"
#include <algorithm>
#include <array>

std::vector<AnyBox::Graphics::Container::EmbeddedDxbc> AnyBox::Graphics::Container::FindEmbeddedDxbc(std::span<const std::byte> data) {
    constexpr std::array magic{std::byte{'D'}, std::byte{'X'}, std::byte{'B'}, std::byte{'C'}};
    std::vector<EmbeddedDxbc> result;
    std::size_t cursor = 0;
    while (cursor < data.size()) {
        auto remaining = data.subspan(cursor);
        auto found = std::search(remaining.begin(), remaining.end(), magic.begin(), magic.end());
        if (found == remaining.end()) break;
        auto offset = cursor + static_cast<std::size_t>(found - remaining.begin());
        cursor = offset + magic.size();
        auto candidate = data.subspan(offset);
        if (candidate.size() < 24 || ReadInteger(candidate, 20) != 1) continue;
        auto size = ReadInteger(candidate, 24);
        if (size < 32 || size > candidate.size()) throw std::invalid_argument("Embedded DXBC container size is invalid");
        auto bytes = candidate.first(size);
        (void)DxbcView(bytes);
        result.push_back({offset, bytes});
        cursor = offset + size;
    }
    return result;
}
