#include <anybox/graphics/container/NativeCompute.hpp>
#include <anybox/graphics/container/Native.hpp>
#include "Integer.hpp"

AnyBox::Graphics::Container::NativeComputeState AnyBox::Graphics::Container::ReadNativeComputeState(std::span<const std::byte> program) {
    FssmView view(program);
    if ((ReadInteger(program, 0x100) & 255) != 6) throw std::invalid_argument("Xbox native compute state belongs to a different hardware stage");
    if (view.GetCodeSectionOffset() + 40 < 0x148) throw std::invalid_argument("Xbox native compute state overlaps its instruction stream");
    NativeComputeState result{};
    result.resourceControl = ReadInteger(program, 0x138);
    result.declaredSharedBytes = ReadInteger(program, 0x10C);
    result.floatControl = ReadInteger(program, 0x104);
    auto flags = std::to_integer<std::uint8_t>(program[0x114]);
    if ((flags & ~3u) != 0) throw std::runtime_error("Xbox native compute common shader flags are not implemented");
    if (result.floatControl > 255) throw std::invalid_argument("Xbox native compute floating mode exceeds its register field");
    result.dx10Clamp = (flags & 1) != 0;
    result.ieeeMode = (flags & 2) != 0;
    std::uint64_t total = 1;
    for (unsigned axis = 0; axis < 3; ++axis) {
        result.threads[axis] = ReadInteger(program, 0x13C + axis * 4);
        if (result.threads[axis] == 0 || result.threads[axis] > 1024) throw std::invalid_argument("Xbox native compute workgroup dimension is invalid");
        total *= result.threads[axis];
    }
    if (total > 1024) throw std::invalid_argument("Xbox native compute workgroup has too many threads");
    auto sharedUnits = (static_cast<std::uint64_t>(result.declaredSharedBytes) + 511) / 512;
    if (sharedUnits != ((result.resourceControl >> 15) & 511)) throw std::invalid_argument("Xbox native compute shared memory declarations disagree");
    return result;
}
