#include <anybox/buffer/AddressSpace.hpp>
#include <random>
#include <stdexcept>

void CheckRandomMappings() {
    using namespace AnyBox::Graphics::Buffers;
    AddressSpace space;
    auto owner = std::make_shared<int>(1);
    std::vector<std::shared_ptr<const Mapping>> entries;
    std::mt19937_64 random(0xAB01);
    for (unsigned iteration = 0; iteration < 10000; ++iteration) {
        if (entries.empty() || random() % 3 == 0) {
            auto begin = 0x1000 + random() % 0x10000;
            auto size = 1 + random() % 0x1000;
            entries.push_back(space.Register(begin, size, 0x100000 + iteration * 0x2000ULL, owner));
        } else if (random() % 4 == 0) {
            auto index = random() % entries.size();
            entries[index].reset();
            entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(index));
        }
        auto address = 0x1000 + random() % 0x12000;
        auto bytes = 1 + random() % 64;
        const Mapping* expected = nullptr;
        for (const auto& mapping : entries) {
            auto begin = mapping->GetGuestAddress();
            auto end = begin + mapping->GetSize();
            if (begin > address || address + bytes > end) continue;
            if (!expected || end > expected->GetGuestAddress() + expected->GetSize() || (end == expected->GetGuestAddress() + expected->GetSize() && begin < expected->GetGuestAddress())) expected = mapping.get();
        }
        try {
            auto actual = space.Resolve(address, bytes);
            if (actual.get() != expected || actual->Translate(address, bytes) != actual->GetDeviceAddress() + address - actual->GetGuestAddress()) throw std::runtime_error("GPU buffer interval lookup disagrees with its reference model");
        } catch (const std::out_of_range&) {
            if (expected) throw std::runtime_error("GPU buffer interval lookup missed a registered range");
        }
    }
    entries.clear();
    if (!space.Capture().mappings.empty()) throw std::runtime_error("GPU buffer interval removal left stale mappings");
}
