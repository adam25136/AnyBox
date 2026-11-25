#include "../native/RootFixture.hpp"
#include "../native/RootDescriptorFixture.hpp"

void BufferTests::CheckNativeArguments(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox::Shader;
    for (unsigned wave : {32u, 64u}) {
        for (unsigned slot : {0u, 2u, 16u, 55u, 255u}) {
            NativeTests::RootFixture fixture(wave, slot);
            RootArgumentPlan plan(fixture.buffers, fixture.root);
            auto translation = fixture.Translate();
            BindNativeRootArguments(translation, fixture.roots, plan);
            auto compiled = CompileCompute(translation.program, translation.input, Target(), {0, 0, 0, 128});
            std::array<std::uint32_t, 16> input{};
            input[0] = 0xA1234567;
            std::vector<std::uint32_t> expected(wave, input[0]);
            RunGpu(context, compiled, expected, {1, 1, 1}, {}, input);
        }
        for (unsigned slot : {0u, 2u, 16u, 54u, 254u}) {
            auto fixture = NativeTests::MakeRootDescriptor(wave, slot);
            RootArgumentPlan plan(fixture.buffers, fixture.root);
            auto translation = fixture.Translate();
            BindNativeRootArguments(translation, fixture.roots, plan);
            auto compiled = CompileCompute(translation.program, translation.input, Target(), {0, 0, 0, 128});
            std::array<std::uint32_t, 16> arguments{};
            Fixtures::Require(translation.program.Resources().srtReads.empty());
            arguments[1] = 1;
            arguments[2] = 4;
            std::array<std::uint32_t, 16> input{};
            input[0] = 0xCAFEBABE;
            std::vector<std::uint32_t> expected(wave, input[0]);
            try { RunGpu(context, compiled, expected, {1, 1, 1}, std::as_bytes(std::span(input)), arguments); }
            catch (const std::exception& error) { throw std::runtime_error("Native descriptor wave=" + std::to_string(wave) + " slot=" + std::to_string(slot) + ": " + error.what() + "\n" + ShaderRecompiler::ProgramToString(translation.program)); }
        }
    }
}
