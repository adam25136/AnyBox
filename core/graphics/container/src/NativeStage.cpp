#include <anybox/graphics/container/NativeStage.hpp>
#include <anybox/graphics/container/Native.hpp>
#include "Integer.hpp"

AnyBox::Graphics::Container::NativeStageState AnyBox::Graphics::Container::ReadNativeStageState(std::span<const std::byte> program) {
    FssmView view(program);
    if (view.GetCodeSectionOffset() + 40 < 0x118) throw std::invalid_argument("Xbox native common shader state overlaps its instruction stream");
    auto hardware = ReadInteger(program, 0x100) & 255;
    if (hardware != 4 && hardware != 5 && hardware != 6) throw std::runtime_error("Xbox native hardware shader stage metadata is not implemented");
    auto mode = ReadInteger(program, 0x104);
    if (mode > 255) throw std::invalid_argument("Xbox native shader floating mode exceeds its register field");
    auto flags = std::to_integer<std::uint8_t>(program[0x114]);
    if ((flags & ~3u) != 0) throw std::runtime_error("Xbox native common shader flags are not implemented");
    return {static_cast<NativeHardwareStage>(hardware), mode, (flags & 1) != 0, (flags & 2) != 0};
}
