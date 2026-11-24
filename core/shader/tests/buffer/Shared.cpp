#include "SharedFixture.hpp"
#include "Target.hpp"
#include "Fixtures.hpp"

namespace {
ShaderRecompiler::ShaderComputeInputInfo Input() {
    ShaderRecompiler::ShaderComputeInputInfo input;
    input.threadsNum[0] = 64;
    input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    return input;
}
}

void BufferTests::CheckSharedGpu(AnyBox::Vulkan::Context& context) {
    {
        SharedFixture fixture;
        auto program = fixture.Lower();
        Fixtures::Require(program.Info().sharedMemoryBytes == 512 && program.Info().scratchDwords == 1);
        auto shader = AnyBox::Shader::CompileCompute(program, Input(), Target(), {0, 0, 0, 128});
        Fixtures::Require(program.Info().sharedMemoryBytes == 0 && shader.bindings.layout.descriptors.size() == 2);
        std::vector<std::uint32_t> expected;
        for (unsigned thread = 0; thread < 128; ++thread) {
            auto group = thread / 64;
            auto local = thread % 64;
            expected.push_back((group * 64 + (local ^ 32)) * 17 + 5);
            expected.push_back(((group * 64 + 63 - local) * 17 + 5) ^ 0xA5A5A5A5);
            expected.push_back(thread + 7);
        }
        RunGpu(context, shader, expected, {2, 1, 1});
    }
    for (std::int32_t index : {-1, 64}) {
        SharedFixture fixture(index);
        Fixtures::Reject<std::out_of_range>([&] { (void)fixture.Lower(); });
    }
    {
        SharedFixture fixture({}, false, 1);
        Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Lower(); });
    }
    {
        SharedFixture fixture;
        auto program = fixture.Lower();
        auto input = Input();
        input.ldsSizeDwords = 127;
        Fixtures::Reject<std::invalid_argument>([&] { (void)AnyBox::Shader::CompileCompute(program, input, Target(), {0, 0, 0, 128}); });
    }
    {
        SharedFixture fixture({}, true);
        auto program = fixture.Lower();
        auto shader = AnyBox::Shader::CompileCompute(program, Input(), Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected(128 * 3);
        bool fault = false;
        try { RunGpu(context, shader, expected, {2, 1, 1}); }
        catch (const std::runtime_error& error) { fault = std::string_view(error.what()).starts_with("DXIL BDA GPU test recorded fault "); }
        Fixtures::Require(fault);
    }
}
