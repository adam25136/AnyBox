#include "IntegerAtomicKernel.hpp"

void CheckIntegerAtomicInvalid();
void CheckIntegerAtomicFaults(AnyBox::Vulkan::Context& context);

namespace {
void CheckPairs(AnyBox::Vulkan::Context& context, bool subtract, unsigned wave) {
    using namespace ShaderRecompiler;
    BufferTests::IntegerAtomicKernel fixture;
    auto& builder = fixture.builder;
    auto& global = fixture.GlobalIndex();
    auto& input = fixture.Memory(IrOpcode::LoadAddressU32, builder.IMul(builder.BitwiseAnd(global, builder.Constant(15)), builder.Constant(4)), 1);
    auto& offset = builder.IMul(global, builder.Constant(4));
    auto& previous = fixture.Modify(subtract, offset, input);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(offset, builder.Constant(1024)), 2, &previous);
    auto shader = fixture.Compile(wave);
    constexpr std::array<std::uint32_t, 16> values{0, 1, 2, 3, UINT32_MAX, UINT32_MAX - 1, 0x80000000, 0x7FFFFFFF, 0xAAAAAAAA, 0x55555555, 0x12345678, 0x87654321, 31, 32, 255, 256};
    std::vector<std::uint32_t> initial(512, UINT32_MAX);
    std::vector<std::uint32_t> expected(512);
    for (unsigned index = 0; index < 256; ++index) {
        initial[index] = values[index / 16];
        expected[index] = subtract ? initial[index] - values[index % 16] : initial[index] + values[index % 16];
        expected[index + 256] = initial[index];
    }
    BufferTests::RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(values)), {}, std::as_bytes(std::span(initial)));
}

void CheckTickets(AnyBox::Vulkan::Context& context, bool subtract, unsigned wave, unsigned threads, std::uint32_t seed) {
    using namespace ShaderRecompiler;
    BufferTests::IntegerAtomicKernel fixture;
    auto& builder = fixture.builder;
    constexpr unsigned groups = 4;
    auto count = threads * groups;
    auto& previous = fixture.Modify(subtract, builder.Constant(0), builder.Constant(1));
    auto& ticket = subtract ? builder.ISub(builder.Constant(seed), previous) : builder.ISub(previous, builder.Constant(seed));
    auto& offset = builder.IMul(builder.IAdd(ticket, builder.Constant(1)), builder.Constant(4));
    fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &previous);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(offset, builder.Constant(count * 4)), 2, &builder.Constant(1));
    auto shader = fixture.Compile(wave, threads);
    std::vector<std::uint32_t> initial(count * 2 + 1, UINT32_MAX);
    std::vector<std::uint32_t> expected(count * 2 + 1, 1);
    initial[0] = seed;
    expected[0] = subtract ? seed - count : seed + count;
    for (unsigned index = 0; index < count; ++index) expected[index + 1] = subtract ? seed - index : seed + index;
    BufferTests::RunGpu(context, shader, expected, {groups, 1, 1}, {}, {}, std::as_bytes(std::span(initial)));
}
}

namespace BufferTests {
void CheckGlobalIntegerAtomic(AnyBox::Vulkan::Context& context) {
    CheckIntegerAtomicInvalid();
    CheckIntegerAtomicFaults(context);
    for (bool subtract : {false, true}) {
        for (unsigned wave : {32u, 64u}) {
            CheckPairs(context, subtract, wave);
            for (std::uint32_t seed : {0u, UINT32_MAX, 0x80000000u, 123456789u}) CheckTickets(context, subtract, wave, 256, seed);
            for (unsigned threads : {1u, 33u, 65u}) CheckTickets(context, subtract, wave, threads, 0);
        }
    }
}
}
