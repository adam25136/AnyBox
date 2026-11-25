#include "GlobalConstantFixture.hpp"
#include "Fixtures.hpp"
#include <string_view>

namespace BufferTests { void CheckGlobalConstants(AnyBox::Vulkan::Context& context); }
void CheckPrivateGlobals(AnyBox::Vulkan::Context& context);

namespace {
AnyBox::Shader::ComputeCompilation Compile(ShaderRecompiler::IrProgram& program) {
    Fixtures::Require(program.Info().scratchDwords == 0 && program.Info().sharedMemoryBytes == 0);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    return AnyBox::Shader::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
}
}

void BufferTests::CheckGlobalConstants(AnyBox::Vulkan::Context& context) {
    CheckPrivateGlobals(context);
    for (unsigned mode = 0; mode < 5; ++mode) {
        for (std::optional<std::int32_t> index : {std::optional<std::int32_t>{}, std::optional<std::int32_t>{0}, std::optional<std::int32_t>{7}}) {
            auto program = MakeConstantKernel(mode, index);
            auto shader = Compile(program);
            std::vector<std::uint32_t> expected(128);
            for (unsigned thread = 0; thread < expected.size(); ++thread) expected[thread] = mode == 0 ? 0x3F800000U : mode >= 3 ? 0 : ConstantWords[index ? static_cast<unsigned>(*index) : thread & 7];
            RunGpu(context, shader, expected, {2, 1, 1});
        }
    }
    for (std::int32_t index : {-1, 8}) Fixtures::Reject<std::out_of_range>([&] { (void)MakeConstantKernel(1, index); });
    Fixtures::Reject<std::runtime_error>([&] { (void)MakeConstantKernel(5); });
    Fixtures::Reject<std::runtime_error>([&] { (void)MakeConstantKernel(1, {}, false, true); });
    auto outside = MakeConstantKernel(1, {}, true);
    auto shader = Compile(outside);
    std::array<std::uint32_t, 128> expected{};
    bool fault = false;
    try { RunGpu(context, shader, expected, {2, 1, 1}); }
    catch (const std::runtime_error& error) { fault = std::string_view(error.what()).starts_with("DXIL BDA GPU test recorded fault "); }
    Fixtures::Require(fault);
}
