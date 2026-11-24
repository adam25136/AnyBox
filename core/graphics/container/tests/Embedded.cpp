#include "Fixtures.hpp"
#include <anybox/graphics/container/Embedded.hpp>
#include <algorithm>
#include <array>

void CheckEmbeddedDxbc() {
    using namespace AnyBox::Graphics::Container;
    const std::array<std::byte, 8> payload{};
    auto container = BuildDxbc(std::array<Part, 1>{{{0x4C495844, payload}}});
    std::vector<std::byte> binary(7, std::byte{0x55});
    binary.insert(binary.end(), container.begin(), container.end());
    binary.resize(binary.size() + 3, std::byte{0xAA});
    auto second = binary.size();
    binary.insert(binary.end(), container.begin(), container.end());
    auto found = FindEmbeddedDxbc(binary);
    Fixtures::Require(found.size() == 2 && found[0].offset == 7 && found[1].offset == second);
    Fixtures::Require(found[0].data.data() == binary.data() + 7 && std::ranges::equal(found[1].data, container));
    Fixtures::Require(FindEmbeddedDxbc({}).empty());
    std::array noise{std::byte{'D'}, std::byte{'X'}, std::byte{'B'}, std::byte{'C'}};
    Fixtures::Require(FindEmbeddedDxbc(noise).empty());
    auto broken = binary;
    broken.pop_back();
    Fixtures::Reject<std::invalid_argument>([&] { (void)FindEmbeddedDxbc(broken); });
    broken = container;
    Fixtures::Set(broken, 32, UINT32_MAX);
    Fixtures::Reject<std::invalid_argument>([&] { (void)FindEmbeddedDxbc(broken); });
    auto nested = BuildDxbc(std::array<Part, 1>{{{0x4C495844, container}}});
    Fixtures::Require(FindEmbeddedDxbc(nested).size() == 1);
}
