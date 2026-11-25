#include <anybox/shader/NativeCompute.hpp>
#include <anybox/shader/ComputeCompiler.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"
#include <Recompiler.hpp>
#include <array>

void CheckSpirvValidation(std::span<const std::uint32_t> code);

namespace {
std::vector<std::byte> Program(std::span<const std::uint32_t> instructions) {
    auto data = Fixtures::MakeComputeProgram();
    data.resize(0x1C0);
    Fixtures::Set(data, 0xC8, 0x80);
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    for (std::size_t index = 0; index < instructions.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, instructions[index]);
    return data;
}
}

void CheckNativeFloatingProfile() {
    using namespace AnyBox::Shader;
    using namespace Fixtures;
    auto target = BufferTests::Target();
    const std::array<std::array<std::uint32_t, 5>, 3> programs{{
        {0xBF800000, 0xBF810000, 0xBF9F0000, 0xBF9F0000, 0xBF9F0000},
        {0xD5408003, 0x00080200, 0xBF810000, 0xBF9F0000, 0xBF9F0000},
        {0x7E060200, 0xDC708000, 0x007F0301, 0xBF810000, 0xBF9F0000}
    }};
    for (const auto& code : programs) {
        auto data = Program(code);
        NativeFrontend frontend(data);
        auto state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
        auto result = CompileNativeCompute(frontend, state, target, {0, 0, 0, 128});
        for (unsigned flags = 0; flags < 8; ++flags) {
            ShaderRecompiler::ShaderFloatMode mode{state.floatControl, (flags & 1) != 0, (flags & 2) != 0, (flags & 4) != 0};
            auto explicitMode = LowerNativeCompute(frontend, state, mode, target);
            auto compiled = CompileCompute(explicitMode.program, explicitMode.input, target, {0, 0, 0, 128});
            Require(compiled.code == result.code && compiled.bindings.layout == result.bindings.layout);
        }
        CheckSpirvValidation(result.code);
    }
    auto data = Program(std::array<std::uint32_t, 5>{0xD5408003, 0x00080200, 0xDC708000, 0x007F0301, 0xBF810000});
    NativeFrontend frontend(data);
    auto state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
    std::vector<std::uint32_t> baseline;
    bool different = false;
    for (unsigned flags = 0; flags < 4; ++flags) {
        Set(data, 0x114, flags);
        state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
        auto result = CompileNativeCompute(frontend, state, target, {0, 0, 0, 128});
        ShaderRecompiler::ShaderFloatMode mode{state.floatControl, (flags & 1) != 0, (flags & 2) != 0, false};
        auto translated = LowerNativeCompute(frontend, state, mode, target);
        auto explicitMode = CompileCompute(translated.program, translated.input, target, {0, 0, 0, 128});
        Require(result.code == explicitMode.code && result.bindings.layout == explicitMode.bindings.layout);
        CheckSpirvValidation(result.code);
        if (flags == 0) baseline = result.code;
        else different |= result.code != baseline;
    }
    Require(different);
    state.floatControl = 256;
    Reject<std::invalid_argument>([&] { (void)CompileNativeCompute(frontend, state, target, {0, 0, 0, 128}); });
}
