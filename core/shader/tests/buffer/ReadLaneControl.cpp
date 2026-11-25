#include "WaveControlKernel.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

void CheckReadLaneReconvergence(AnyBox::Vulkan::Context& context);

void CheckReadLaneControl(AnyBox::Vulkan::Context& context) {
    CheckReadLaneReconvergence(context);
    using namespace ShaderRecompiler;
    ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.hostSubgroupSize = 32;
    for (unsigned wave : {32u, 64u}) {
        info.waveSize = wave;
        for (unsigned mode : {2u, 6u}) {
            auto program = BufferTests::WaveControlKernel(mode, true);
            program.SetWaveSize(wave);
            auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
            std::vector<std::uint32_t> expected;
            for (unsigned index = 0; index < 256; ++index) {
                bool active = mode == 2 ? index >= 128 : index % 128 < 64;
                expected.push_back(active ? index + index / wave * wave : index ^ 0xA55A8420);
            }
            BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
        }
        auto program = BufferTests::WaveControlKernel(8, true);
        program.SetWaveSize(wave);
        bool rejected = false;
        try { (void)AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128}); }
        catch (const std::runtime_error& error) { rejected = std::string_view(error.what()) == "Xbox read lane from divergent control flow is not implemented"; }
        Fixtures::Require(rejected);
    }
}
