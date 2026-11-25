#include "WideStorageFixture.hpp"
#include "Fixtures.hpp"

namespace BufferTests {
void CheckWideStorage(AnyBox::Vulkan::Context& context) {
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    for (unsigned wave : {32u, 64u}) {
        info.waveSize = wave;
        for (unsigned mode : {0u, 1u, 6u, 7u}) {
            auto program = WideStorageKernel(mode);
            program.SetWaveSize(wave);
            Fixtures::Require(program.Info().sharedMemoryBytes == 512 && program.Info().scratchDwords == 128);
            auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
            std::vector<std::uint32_t> expected;
            for (unsigned thread = 0; thread < 128; ++thread) {
                auto selected = mode == 1 ? (thread / 64) * 64 + 63 : thread ^ 32;
                for (auto value : {selected, thread}) {
                    expected.push_back(value * 17 + 5);
                    expected.push_back(value ^ 0xA5F03C69);
                }
                auto constant = mode >= 6 ? 0 : WideConstants[mode == 1 ? 7 : thread & 7];
                expected.push_back(static_cast<std::uint32_t>(constant));
                expected.push_back(static_cast<std::uint32_t>(constant >> 32));
            }
            RunGpu(context, shader, expected, {2, 1, 1});
        }
        auto program = WideStorageKernel(4);
        program.SetWaveSize(wave);
        auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected(128 * 6);
        bool fault = false;
        try { RunGpu(context, shader, expected, {2, 1, 1}); }
        catch (const std::runtime_error& error) { fault = std::string_view(error.what()).starts_with("DXIL BDA GPU test recorded fault "); }
        Fixtures::Require(fault);
    }
    for (unsigned mode : {2u, 3u, 5u}) Fixtures::Reject<std::out_of_range>([&] { (void)WideStorageKernel(mode); });
}
}
