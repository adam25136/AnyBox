#include "UniformLoopKernel.hpp"
#include "../../src/backend/control/Uniformity.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

void CheckUniformLoopProof();

namespace BufferTests {
void CheckUniformLoop(AnyBox::Vulkan::Context& context) {
    CheckUniformLoopProof();
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 256;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    for (unsigned mode : {0u, 1u}) {
        for (bool readLane : {false, true}) {
            auto source = UniformLoopKernel(mode, readLane);
            AnyBox::Shader::Control::Uniformity uniform(source.program, info);
            Fixtures::Require(uniform.Check(*source.counter));
            if (readLane) Fixtures::Require(uniform.Check(*source.broadcast));
            Fixtures::Require(!uniform.Check(*source.sum));
            Fixtures::Require(uniform.Check(*source.counter));
            auto shader = AnyBox::Shader::Dxil::CompileCompute(source.program, info, Target(), {0, 0, 0, 128});
            std::vector<std::uint32_t> expected;
            for (unsigned index = 0; index < 512; ++index) {
                auto iterations = index % 256 / 64 + 1;
                auto start = mode == 1 ? index / 256 : 0;
                auto sum = 17u;
                for (unsigned iteration = start; iteration < start + iterations; ++iteration) sum += index * 17 + 5 + (readLane ? index / 64 * 64 + iteration : 0);
                expected.push_back(sum);
                expected.push_back(start + iterations);
            }
            RunGpu(context, shader, expected, {2, 1, 1});
        }
    }
    for (unsigned mode : {2u, 3u, 4u, 5u}) {
        auto source = UniformLoopKernel(mode);
        AnyBox::Shader::Control::Uniformity uniform(source.program, info);
        Fixtures::Require(!uniform.Check(*source.counter));
        Fixtures::Require(!uniform.Check(*source.sum));
        Fixtures::Require(!uniform.Check(*source.counter));
        bool rejected = false;
        try { (void)AnyBox::Shader::Dxil::CompileCompute(source.program, info, Target(), {0, 0, 0, 128}); }
        catch (const std::runtime_error& error) { rejected = std::string_view(error.what()) == "Wave64 per-invocation control flow on a 32-lane subgroup is not implemented"; }
        Fixtures::Require(rejected);
    }
    auto temporal = UniformLoopKernel(5, true);
    AnyBox::Shader::Control::Uniformity uniform(temporal.program, info);
    Fixtures::Require(!uniform.Check(*temporal.broadcast));
    Fixtures::Require(!uniform.Check(*temporal.counter));
    Fixtures::Require(!uniform.Check(*temporal.broadcast));
}
}
