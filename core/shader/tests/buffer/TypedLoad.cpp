#include "Fixture.hpp"
#include "Fixtures.hpp"
#include "Target.hpp"

namespace {
std::uint32_t Expected(unsigned bits, unsigned offset, bool signedInput) {
    if (static_cast<std::uint64_t>(offset) + bits / 8 > 64) return 0;
    std::uint32_t value = 0;
    for (unsigned index = 0; index < bits / 8; ++index) value |= ((offset + index) * 3 + 7) << (index * 8);
    if (signedInput && bits < 32 && (value & (1u << (bits - 1)))) value |= UINT32_MAX << bits;
    return value;
}

void CheckLoad(AnyBox::Vulkan::Context& context, unsigned bits, bool signedInput, unsigned offset, unsigned category) {
    BufferTests::Fixture fixture(false, category);
    auto format = ((signedInput ? 5 : 4) << 4) | (bits / 8);
    auto* loaded = fixture.TypedLoad(format, offset);
    fixture.WriteOutput(fixture.Extract(loaded, 0));
    auto& program = fixture.Finish();
    ShaderRecompiler::ShaderComputeInputInfo input;
    input.threadsNum[0] = input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, input, BufferTests::Target(), {0, 0, 0, 128});
    std::array expected{Expected(bits, offset, signedInput)};
    BufferTests::RunGpu(context, shader, expected, {1, 1, 1});
}
}

void BufferTests::CheckTypedLoadGpu(AnyBox::Vulkan::Context& context) {
    for (unsigned bits : {8u, 16u, 32u}) {
        for (bool signedInput : {false, true}) {
            for (unsigned offset : {1u, 52u, 64u - bits / 8, 65u - bits / 8, UINT32_MAX}) {
                CheckLoad(context, bits, signedInput, offset, 0);
            }
        }
    }
    CheckLoad(context, 8, false, 63, 1);
    for (unsigned format : {0u, 1u, 0x43u, 0x71u, 0x141u}) {
        Fixture fixture(false);
        Fixtures::Reject<std::runtime_error>([&] { (void)fixture.TypedLoad(format, 0); });
    }
    {
        Fixture structured(true);
        Fixtures::Reject<std::runtime_error>([&] { (void)structured.TypedLoad(65, 0); });
    }
    Fixture scalar(false);
    auto* loaded = scalar.TypedLoad(65, 0);
    for (unsigned component = 1; component < 5; ++component) {
        Fixtures::Reject<std::runtime_error>([&] { (void)scalar.Extract(loaded, component); });
    }
}
