#pragma once

#include "View.hpp"

namespace AnyBox::Graphics::Container {
struct EmbeddedDxbc {
    std::size_t offset;
    std::span<const std::byte> data;
};

std::vector<EmbeddedDxbc> FindEmbeddedDxbc(std::span<const std::byte> data);
}
