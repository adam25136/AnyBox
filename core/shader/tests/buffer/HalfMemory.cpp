#include "HalfMemoryFixture.hpp"
#include "ComputeCompiler.hpp"
#include "Target.hpp"
#include "Fixtures.hpp"

void CheckHalfMemory(AnyBox::Vulkan::Context& context) {
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.hostSubgroupSize = 32;
    for (unsigned wave : {32u, 64u}) {
        info.waveSize = wave;
        for (bool floating : {false, true}) {
            for (unsigned mode = 0; mode < 4; ++mode) {
                auto count = mode == 0 ? 3u : mode == 1 ? 65u : 9u;
                for (auto fixed : {std::optional<std::int32_t>{}, std::optional<std::int32_t>{0}, std::optional<std::int32_t>{static_cast<std::int32_t>(count - 1)}}) {
                    BufferTests::HalfMemoryFixture fixture(floating, mode, fixed);
                    auto program = fixture.Lower(wave);
                    if (mode == 0) Fixtures::Require(program.Info().scratchDwords == 3);
                    if (mode == 1) Fixtures::Require(program.Info().sharedMemoryBytes == 136);
                    if (mode == 2) Fixtures::Require(program.Info().scratchDwords == 6);
                    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
                    std::vector<std::uint32_t> expected;
                    for (unsigned index = 0; index < 128; ++index) {
                        auto selected = fixed ? static_cast<unsigned>(*fixed) : mode == 1 ? ((index % 64) ^ 1) + 1 : index % count;
                        if (mode == 0) {
                            expected.push_back((index * 17 + 0x7B00 + selected) & 65535);
                            expected.push_back((index * 17 + 0x7B00 + (selected + 1) % count) & 65535);
                        } else if (mode == 1) {
                            expected.push_back(selected == 0 ? 0xD35A : ((index / 64 * 64 + selected - 1) * 17 + 0x7B00) & 65535);
                            expected.push_back(0xD35A);
                        } else {
                            expected.push_back(BufferTests::HalfMemoryWords[selected]);
                            expected.push_back(mode == 2 ? (index * 17 + 0x7B00) & 65535 : BufferTests::HalfMemoryWords[(selected + 1) % count]);
                        }
                        expected.push_back(0x7D55);
                    }
                    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
                }
                for (auto index : {-1, static_cast<std::int32_t>(count)}) {
                    BufferTests::HalfMemoryFixture fixture(floating, mode, index);
                    Fixtures::Reject<std::out_of_range>([&] { (void)fixture.Lower(wave); });
                }
            }
        }
    }
    BufferTests::HalfMemoryFixture outside(true, 1, {}, true);
    auto program = outside.Lower(32);
    info.waveSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    bool fault = false;
    try { BufferTests::RunGpu(context, shader, std::vector<std::uint32_t>(128 * 3), {2, 1, 1}); }
    catch (const std::runtime_error& error) { fault = std::string_view(error.what()).starts_with("DXIL BDA GPU test recorded fault "); }
    Fixtures::Require(fault);
}
