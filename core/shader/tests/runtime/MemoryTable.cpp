#include <anybox/shader/runtime/MemoryTable.hpp>
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

using namespace AnyBox::Shader::Runtime;
using AnyBox::Graphics::Buffers::AddressSpace;
namespace Abi = ShaderRecompiler::BdaAbi;
void CheckRandomMemoryTable();

namespace {
void Require(bool condition) { if (!condition) throw std::runtime_error("GPU memory table test failed"); }
template<class TFunction>
void Reject(TFunction&& function) {
    try { function(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Invalid GPU memory table was accepted");
}
}

void CheckMemoryTable() {
    AddressSpace addresses;
    auto owner = std::make_shared<int>(1);
    auto first = addresses.Register(0x1000, 128, 0x8000, owner);
    auto second = addresses.Register(0x1040, 128, 0x9000, owner);
    auto contained = addresses.Register(0x1020, 32, 0xA000, owner);
    auto duplicate = addresses.Register(0x1000, 64, 0xB000, owner);
    std::array bindings{MemoryBinding{second}, MemoryBinding{contained}, MemoryBinding{duplicate}, MemoryBinding{first}};
    {
        MemoryTable table(bindings);
        auto ranges = table.GetRanges();
        Require(ranges.size() == 2 && ranges[0].begin == 0x1000 && ranges[0].end == 0x1080 && ranges[0].deviceAddress == 0x8000);
        Require(ranges[1].begin == 0x1040 && ranges[1].end == 0x10C0 && ranges[1].deviceAddress == 0x9000);
        Require(0x103C + 8 <= ranges[0].end);
        std::vector<std::byte> encoded(table.GetByteSize());
        table.Write(encoded);
        Abi::Header header;
        std::memcpy(&header, encoded.data(), sizeof(header));
        Require(header.version == Abi::Version && header.count == 2 && header.entryBytes == 32 && header.reserved == 0);
        Require(std::memcmp(encoded.data() + sizeof(header), ranges.data(), ranges.size_bytes()) == 0);
        Reject([&] { table.Write(std::span(encoded).first(encoded.size() - 1)); });
        for (auto& binding : bindings) binding.mapping.reset();
        first.reset(); second.reset(); contained.reset(); duplicate.reset();
        Require(addresses.Capture().mappings.size() == 2);
    }
    Require(addresses.Capture().mappings.empty());
    MemoryTable empty({});
    Require(empty.GetRanges().empty() && empty.GetByteSize() == sizeof(Abi::Header));
    std::array<std::byte, sizeof(Abi::Header)> bytes;
    empty.Write(bytes);
    Abi::Header header;
    std::memcpy(&header, bytes.data(), bytes.size());
    Require(header.count == 0 && header.version == Abi::Version);

    auto mapping = addresses.Register(0x1000, 128, 0x8000, owner);
    for (auto permissions : {0u, 4u, UINT32_MAX}) {
        std::array invalid{MemoryBinding{mapping, permissions}};
        Reject([&] { MemoryTable selected(invalid); });
    }
    std::array missing{MemoryBinding{}};
    Reject([&] { MemoryTable selected(missing); });
    auto overlap = addresses.Register(0x1040, 128, 0x9000, owner);
    std::array conflict{MemoryBinding{mapping, Abi::Read}, MemoryBinding{overlap, Abi::Write}};
    Reject([&] { MemoryTable selected(conflict); });
    auto adjacent = addresses.Register(0x1080, 64, 0xA000, owner);
    std::array separate{MemoryBinding{mapping, Abi::Read}, MemoryBinding{adjacent, Abi::Write}};
    MemoryTable permissions(separate);
    Require(permissions.GetRanges().size() == 2 && permissions.GetRanges()[0].permissions == Abi::Read && permissions.GetRanges()[1].permissions == Abi::Write);
    CheckRandomMemoryTable();
}
