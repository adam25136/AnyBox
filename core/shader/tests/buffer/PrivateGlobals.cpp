#include "PrivateGlobalFixture.hpp"
#include "Target.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

namespace {
void CheckValues(AnyBox::Vulkan::Context& context, const ShaderRecompiler::ShaderComputeInputInfo& info) {
    for (unsigned mode = 0; mode < 5; ++mode) {
        for (bool expressions : {false, true}) {
            for (std::optional<std::int32_t> index : {std::optional<std::int32_t>{}, std::optional<std::int32_t>{0}, std::optional<std::int32_t>{7}}) {
                BufferTests::PrivateGlobalFixture fixture(mode, expressions, index);
                auto program = fixture.Lower();
                Fixtures::Require(program.Info().scratchDwords == (mode == 3 ? 18u : 10u));
                Fixtures::Require(program.Info().sharedMemoryBytes == 0);
                auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
                std::vector<std::uint32_t> expected;
                for (unsigned thread = 0; thread < 128; ++thread) {
                    bool written = mode == 4 || (thread & 1) != 0;
                    auto initial = mode == 0 ? 0 : BufferTests::ConstantWords[index ? static_cast<unsigned>(*index) : thread & 7];
                    expected.push_back(written ? thread * 17 + 5 : initial);
                    expected.push_back(mode != 3 ? 0 : written ? 0xDEADBEEF : initial ^ 0xA55A8420);
                    expected.push_back(written ? thread : 0x12345678);
                }
                BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
            }
        }
    }
}
}

void CheckPrivateGlobals(AnyBox::Vulkan::Context& context) {
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.hostSubgroupSize = 32;
    for (unsigned wave : {32u, 64u}) {
        info.waveSize = wave;
        CheckValues(context, info);
    }
    info.waveSize = 32;
    for (unsigned mode : {5u, 6u, 7u, 8u, 9u}) {
        BufferTests::PrivateGlobalFixture fixture(mode, true);
        Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Lower(); });
    }
    for (std::int32_t index : {-1, 8}) {
        BufferTests::PrivateGlobalFixture fixture(1, false, index);
        Fixtures::Reject<std::out_of_range>([&] { (void)fixture.Lower(); });
    }
    BufferTests::PrivateGlobalFixture fixture(2, true, {}, true);
    auto program = fixture.Lower();
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected(128 * 3);
    bool fault = false;
    try { BufferTests::RunGpu(context, shader, expected, {2, 1, 1}); }
    catch (const std::runtime_error& error) { fault = std::string_view(error.what()).starts_with("DXIL BDA GPU test recorded fault "); }
    Fixtures::Require(fault);
}
