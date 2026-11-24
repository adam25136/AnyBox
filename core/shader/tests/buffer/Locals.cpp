#include "LocalFixture.hpp"
#include "Target.hpp"
#include "Fixtures.hpp"
#include <bit>

namespace {
ShaderRecompiler::ShaderComputeInputInfo Input() {
    ShaderRecompiler::ShaderComputeInputInfo input;
    input.threadsNum[0] = 32;
    input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    return input;
}
}

void BufferTests::CheckLocals(AnyBox::Vulkan::Context& context) {
    for (unsigned pointerMode : {0u, 1u}) {
        LocalFixture fixture({}, false, pointerMode);
        auto program = fixture.Lower();
        Fixtures::Require(program.Info().scratchDwords == 580);
        auto shader = AnyBox::Shader::CompileCompute(program, Input(), Target(), {0, 0, 0, 128});
        Fixtures::Require(shader.info.scratchPoolSlotDwords == 0 && shader.bindings.layout.descriptors.size() == 2);
        std::uint32_t scratch = 0;
        bool functionStorage = false;
        for (std::size_t offset = 5; offset < shader.code.size();) {
            auto size = shader.code[offset] >> 16;
            auto operation = shader.code[offset] & 65535;
            Fixtures::Require(size != 0 && size <= shader.code.size() - offset);
            if (operation == spv::OpName && size > 2) {
                auto name = std::string_view(reinterpret_cast<const char*>(shader.code.data() + offset + 2), (size - 2) * 4);
                if (name.starts_with("scratch_dwords")) scratch = shader.code[offset + 1];
            }
            if (operation == spv::OpVariable && size >= 4 && shader.code[offset + 2] == scratch && shader.code[offset + 3] == spv::StorageClassFunction) functionStorage = true;
            offset += size;
        }
        Fixtures::Require(scratch != 0 && functionStorage);
        std::vector<std::uint32_t> expected;
        for (unsigned invocation = 0; invocation < 64; ++invocation) {
            expected.push_back(std::bit_cast<std::uint32_t>(((invocation * 37) % 576) * 0.25f));
            expected.push_back(invocation * 17 + invocation % 4);
            expected.push_back(invocation);
        }
        RunGpu(context, shader, expected, {2, 1, 1});
    }
    {
        LocalFixture fixture({}, false, 2);
        Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Lower(); });
    }
    for (std::int32_t index : {-1, 576}) {
        LocalFixture fixture(index);
        Fixtures::Reject<std::out_of_range>([&] { (void)fixture.Lower(); });
    }
    {
        LocalFixture fixture({}, true);
        auto program = fixture.Lower();
        auto shader = AnyBox::Shader::CompileCompute(program, Input(), Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected(64 * 3);
        bool fault = false;
        try { RunGpu(context, shader, expected, {2, 1, 1}); }
        catch (const std::runtime_error& error) { fault = std::string_view(error.what()).starts_with("DXIL BDA GPU test recorded fault "); }
        Fixtures::Require(fault);
    }
}
