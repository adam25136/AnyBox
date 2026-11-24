#include "BufferAtomicFixture.hpp"
#include <algorithm>
#include <bit>

namespace BufferTests { void CheckBufferAtomicInvalid(); }

namespace {
std::uint32_t Expected(unsigned mode, std::uint32_t old, std::uint32_t value) {
    switch (mode) {
        case 0: return old + value;
        case 1: return old & value;
        case 2: return old | value;
        case 3: return old ^ value;
        case 4: return std::bit_cast<std::uint32_t>(std::min(std::bit_cast<std::int32_t>(old), std::bit_cast<std::int32_t>(value)));
        case 5: return std::bit_cast<std::uint32_t>(std::max(std::bit_cast<std::int32_t>(old), std::bit_cast<std::int32_t>(value)));
        case 6: return std::min(old, value);
        case 7: return std::max(old, value);
        case 8: return value;
        default: throw std::logic_error("Unknown atomic test mode");
    }
}

void CheckPairs(AnyBox::Vulkan::Context& context, unsigned mode, bool structured) {
    using namespace ShaderRecompiler;
    BufferTests::BufferAtomicFixture fixture(structured);
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto arguments = fixture.Arguments(mode);
    auto& byteOffset = builder.ShiftLeftLogical(index, builder.Constant(2));
    arguments[3] = fixture.Bind(structured ? builder.ShiftRightLogical(index, builder.Constant(1)) : byteOffset);
    if (structured) arguments[4] = fixture.Bind(builder.ShiftLeftLogical(builder.BitwiseAnd(index, builder.Constant(1)), builder.Constant(2)));
    auto& inputOffset = builder.ShiftLeftLogical(builder.BitwiseAnd(index, builder.Constant(15)), builder.Constant(2));
    auto& value = fixture.Memory(IrOpcode::LoadAddressU32, inputOffset, 1);
    arguments[6] = fixture.Bind(value);
    auto& previous = fixture.scalar.Get(*fixture.Call(std::move(arguments)));
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(byteOffset, builder.Constant(1024)), 2, &previous);
    auto shader = fixture.Compile();
    std::array<std::uint32_t, 16> values{0, 1, 2, 3, 7, 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF, 0xAAAAAAAA, 0x55555555, 0xFFFFFFFE, 0x80000001, 31, 32, 255, 256};
    std::vector<std::uint32_t> initial(512, UINT32_MAX);
    std::vector<std::uint32_t> expected(512);
    for (unsigned sample = 0; sample < 256; ++sample) {
        initial[sample] = values[sample / 16];
        expected[sample] = Expected(mode, initial[sample], values[sample % 16]);
        expected[sample + 256] = initial[sample];
    }
    BufferTests::RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(values)), {}, std::as_bytes(std::span(initial)));
}

void CheckConcurrent(AnyBox::Vulkan::Context& context, bool bitwise) {
    using namespace ShaderRecompiler;
    BufferTests::BufferAtomicFixture fixture(false);
    auto arguments = fixture.Arguments(bitwise ? 2 : 0);
    auto& builder = fixture.builder;
    auto& lane = builder.BitwiseAnd(fixture.GlobalIndex(), builder.Constant(31));
    arguments[6] = bitwise ? fixture.Bind(builder.ShiftLeftLogical(builder.Constant(1), lane)) : fixture.Integer(32, 1);
    (void)fixture.scalar.Get(*fixture.Call(std::move(arguments)));
    auto shader = fixture.Compile();
    std::array initial{0u};
    std::array expected{bitwise ? UINT32_MAX : 1024u};
    BufferTests::RunGpu(context, shader, expected, {16, 1, 1}, {}, {}, std::as_bytes(std::span(initial)));
}
}

void BufferTests::CheckBufferAtomicGpu(AnyBox::Vulkan::Context& context) {
    CheckBufferAtomicInvalid();
    for (unsigned mode = 0; mode < 9; ++mode) {
        CheckPairs(context, mode, false);
        CheckPairs(context, mode, true);
    }
    CheckConcurrent(context, false);
    CheckConcurrent(context, true);
}
