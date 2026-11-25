#include "WaveControlKernel.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

namespace BufferTests {
void CheckWave64Control(AnyBox::Vulkan::Context& context) {
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    for (unsigned mode = 0; mode < 4; ++mode) {
        auto program = WaveControlKernel(mode);
        auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (unsigned index = 0; index < 256; ++index) {
            bool active = mode == 1 || (mode == 2 && (index / 128) % 2 != 0) || (mode == 3 && (index & 64) != 0);
            expected.push_back(active ? index + index / 64 * 64 : index ^ 0xA55A8420);
        }
        RunGpu(context, shader, expected, {2, 1, 1});
    }
    for (unsigned mode : {4u, 5u}) {
        auto program = WaveControlKernel(mode);
        bool rejected = false;
        try { (void)AnyBox::Shader::Dxil::CompileCompute(program, info, Target(), {0, 0, 0, 128}); }
        catch (const std::runtime_error& error) { rejected = std::string_view(error.what()) == "Wave64 per-invocation control flow on a 32-lane subgroup is not implemented"; }
        Fixtures::Require(rejected);
    }
}
}
