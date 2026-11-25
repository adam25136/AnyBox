#include <anybox/graphics/container/NativeInterface.hpp>
#include <anybox/graphics/container/NativeStage.hpp>
#include <anybox/graphics/container/Native.hpp>
#include "../Integer.hpp"

namespace {
std::size_t TableOffset(std::span<const std::byte> program, AnyBox::Graphics::Container::NativeHardwareStage expected, std::size_t offset, std::size_t size) {
    using namespace AnyBox::Graphics::Container;
    FssmView view(program);
    if (ReadNativeStageState(program).stage != expected) throw std::invalid_argument("Xbox native interface belongs to a different hardware stage");
    auto version = ReadInteger(program, 0x10);
    if (version != 5 && version != 6) throw std::runtime_error("Xbox native shader interface version is not implemented");
    if (version == 6) offset += 8;
    if (offset + size > view.GetCodeSectionOffset()) throw std::invalid_argument("Xbox native shader interface overlaps its code section");
    return offset;
}
}

AnyBox::Graphics::Container::NativePixelInputs AnyBox::Graphics::Container::ReadNativePixelInputs(std::span<const std::byte> program) {
    auto base = TableOffset(program, NativeHardwareStage::Pixel, 0x130, 0x114);
    NativePixelInputs result;
    result.count = ReadInteger(program, base + 0x100);
    result.inputEnable = ReadInteger(program, base + 0x110);
    if (result.count > result.attributes.size()) throw std::invalid_argument("Xbox native pixel attribute count exceeds its table");
    if ((result.inputEnable & ~0xFFFFu) != 0) throw std::runtime_error("Xbox native pixel input enable bits are not implemented");

    for (std::uint32_t index = 0; index < result.count; ++index) {
        auto record = program.subspan(base + index * 8, 8);
        auto byte = [&](std::size_t field) { return std::to_integer<std::uint8_t>(record[field]); };
        if (byte(3) != index || byte(4) != 0 || byte(5) != 0 || byte(6) != byte(1)) throw std::invalid_argument("Xbox native pixel attribute record is inconsistent");
        if (byte(2) == 0 || (byte(2) & ~15u) != 0) throw std::invalid_argument("Xbox native pixel attribute component mask is invalid");
        if (byte(7) != 1 && byte(7) != 2) throw std::runtime_error("Xbox native pixel attribute interpolation is not implemented");
        result.attributes[index] = {byte(0), byte(1), byte(2), byte(7)};
    }
    return result;
}

AnyBox::Graphics::Container::NativeVertexOutputs AnyBox::Graphics::Container::ReadNativeVertexOutputs(std::span<const std::byte> program) {
    auto base = TableOffset(program, NativeHardwareStage::Vertex, 0x1D4, 0xC4);
    NativeVertexOutputs result;
    result.count = ReadInteger(program, base + 0xC0);
    if (result.count > result.parameters.size()) throw std::invalid_argument("Xbox native vertex parameter count exceeds its table");

    for (std::uint32_t index = 0; index < result.count; ++index) {
        auto record = program.subspan(base + index * 6, 6);
        auto byte = [&](std::size_t field) { return std::to_integer<std::uint8_t>(record[field]); };
        if (byte(2) != index || byte(3) != 0 || byte(4) != 0 || byte(5) != 0) throw std::invalid_argument("Xbox native vertex parameter record is inconsistent");
        result.parameters[index] = {byte(0), byte(1)};
    }
    return result;
}
