#include <anybox/shader/runtime/MemoryTable.hpp>
#include <algorithm>
#include <random>
#include <stdexcept>

void CheckRandomMemoryTable() {
    using namespace AnyBox::Shader::Runtime;
    AnyBox::Graphics::Buffers::AddressSpace addresses;
    auto owner = std::make_shared<int>(1);
    std::mt19937_64 random(0xAB02);
    for (unsigned iteration = 0; iteration < 100; ++iteration) {
        std::vector<MemoryBinding> bindings;
        for (unsigned index = 0; index < 100; ++index) {
            auto begin = 0x1000 + random() % 0x10000;
            auto size = 1 + random() % 0x2000;
            bindings.push_back({addresses.Register(begin, size, 0x100000 + index * 0x2000, owner)});
        }
        MemoryTable table(bindings);
        auto ranges = table.GetRanges();
        if (!ShaderRecompiler::BdaAbi::RangesOrdered(ranges.data(), ranges.size())) throw std::runtime_error("GPU memory table ranges are unordered");
        for (unsigned query = 0; query < 1000; ++query) {
            auto address = 0x1000 + random() % 0x12000;
            auto bytes = 1 + random() % 32;
            const AnyBox::Graphics::Buffers::Mapping* expected = nullptr;
            for (const auto& binding : bindings) {
                const auto& mapping = *binding.mapping;
                auto begin = mapping.GetGuestAddress();
                auto end = begin + mapping.GetSize();
                if (begin > address || address + bytes > end) continue;
                if (!expected || end > expected->GetGuestAddress() + expected->GetSize() || (end == expected->GetGuestAddress() + expected->GetSize() && begin < expected->GetGuestAddress())) expected = &mapping;
            }
            auto selected = std::upper_bound(ranges.begin(), ranges.end(), address, [](auto value, const auto& range) { return value < range.begin; });
            std::uint64_t actual = 0;
            if (selected != ranges.begin()) {
                --selected;
                if (address + bytes <= selected->end) actual = selected->deviceAddress + address - selected->begin;
            }
            if (actual != (expected ? expected->Translate(address, bytes) : 0)) throw std::runtime_error("GPU memory table lookup disagrees with a complete interval search");
        }
    }
}
