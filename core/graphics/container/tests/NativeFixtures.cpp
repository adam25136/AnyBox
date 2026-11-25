#include "NativeFixtures.hpp"
#include <anybox/graphics/container/View.hpp>
#include <libdeflate.h>
#include <algorithm>
#include <array>
#include <memory>

std::vector<std::byte> Fixtures::MakeNativeProgram() {
    std::vector<std::byte> data(0x180);
    Set(data, 0, 0x4D535346);
    Set(data, 4, 0x101);
    Set(data, 0xC0, 0x140);
    Set(data, 0xC8, 0x40);
    Set(data, 0xD4, 0x20);
    Set(data, 0x168, 0xBF800000);
    Set(data, 0x16C, 0xBF810000);
    for (unsigned offset = 0x170; offset < data.size(); offset += 4) Set(data, offset, 0xBF9F0000);
    return data;
}

std::vector<std::byte> Fixtures::PackNativeProgram(std::span<const std::byte> data) {
    std::unique_ptr<libdeflate_compressor, decltype(&libdeflate_free_compressor)> compressor(libdeflate_alloc_compressor(1), &libdeflate_free_compressor);
    if (!compressor) throw std::bad_alloc();
    auto bound = libdeflate_zlib_compress_bound(compressor.get(), data.size());
    std::vector<std::byte> output(24 + bound);
    auto size = libdeflate_zlib_compress(compressor.get(), data.data(), data.size(), output.data() + 24, bound);
    if (size == 0) throw std::runtime_error("Cannot compress native shader fixture");
    output.resize(24 + size);
    Set(output, 0, 0x48434258);
    Set(output, 4, 3);
    Set(output, 8, static_cast<std::uint32_t>(size));
    Set(output, 16, static_cast<std::uint32_t>(data.size()));
    return output;
}

std::vector<std::byte> Fixtures::MakeComputeProgram() {
    auto data = MakeNativeProgram();
    Set(data, 0x100, 6);
    Set(data, 0x104, 0xC0);
    Set(data, 0x10C, 513);
    Set(data, 0x114, 1);
    Set(data, 0x138, (2 << 15) | (2 << 11) | 0x380 | 8);
    Set(data, 0x13C, 4);
    Set(data, 0x140, 2);
    Set(data, 0x144, 2);
    return data;
}

std::vector<std::byte> Fixtures::MakeNativeContainer(std::span<const std::byte> first, std::span<const std::byte> second) {
    std::vector<std::byte> program(72);
    Set(program, 0, 0x50066);
    Set(program, 8, 0x4C495844);
    Set(program, 12, 0x106);
    Set(program, 24, 0x54414458);
    Set(program, 28, 3);
    Set(program, 32, 36);
    std::array<std::byte, 28> inner{};
    Set(inner, 0, 0x50066);
    Set(inner, 4, 7);
    Set(inner, 8, 0x4C495844);
    Set(inner, 12, 0x106);
    Set(inner, 16, 16);
    Set(inner, 20, 4);
    Set(inner, 24, 0x43424F4E);
    std::array<std::span<const std::byte>, 3> entries{inner, first, second};
    for (unsigned index = 0; index < entries.size(); ++index) {
        auto offset = program.size();
        Set(program, 36 + index * 12, index == 0 ? 7 : 13);
        Set(program, 40 + index * 12, static_cast<std::uint32_t>(offset));
        Set(program, 44 + index * 12, static_cast<std::uint32_t>(entries[index].size()));
        program.insert(program.end(), entries[index].begin(), entries[index].end());
        program.resize((program.size() + 3) & ~std::size_t{3});
        if (index == 0) Set(program, 16, static_cast<std::uint32_t>(offset + 16));
    }
    Set(program, 4, static_cast<std::uint32_t>(program.size() / 4));
    Set(program, 20, 4);
    return AnyBox::Graphics::Container::BuildDxbc(std::array<AnyBox::Graphics::Container::Part, 1>{{{0x4C495844, program}}});
}
