#include "Target.hpp"
#include "NativeFixtures.hpp"
#include <anybox/shader/NativeCompute.hpp>
#include <Recompiler.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <vector>

namespace {
AnyBox::Shader::ComputeCompilation Compile(unsigned modifier, bool clamp, bool ieee, unsigned window) {
    auto data = Fixtures::MakeComputeProgram();
    Fixtures::Set(data, 0x13C, 32);
    Fixtures::Set(data, 0x140, 1);
    Fixtures::Set(data, 0x144, 1);
    std::vector<std::uint32_t> code{0x7E0C0280, 0x7E0E0281, 0xDC3C8000, 0x037F0006, 0x7E040282, 0x34020082};
    std::array<std::uint32_t, 2> fma{0xD5400003 | (static_cast<unsigned>(clamp) << 15), 259 | (260 << 9) | (261 << 18) | (modifier << 27)};
    if (window != 0) code.push_back(0xBEFC0304);
    if (window == 3) code.insert(code.end(), fma.begin(), fma.end());
    if (window == 2 || window == 3 || window == 5) code.push_back(0xBF820000);
    if (window != 0) code.push_back(window >= 4 ? 0x7E088704 : 0x7E068503);
    if (window != 3) code.insert(code.end(), fma.begin(), fma.end());
    if (window != 0) code.push_back(0x7E068703);
    if (window == 2 || window == 3 || window == 5) code.push_back(0xBF820000);
    code.insert(code.end(), {0xDC708000, 0x007F0301, 0xBF810000});
    auto bytes = (40 + code.size() * 4 + 15) & ~std::size_t{15};
    data.resize(0x140 + bytes);
    Fixtures::Set(data, 0xC8, static_cast<std::uint32_t>(bytes));
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    for (std::size_t index = 0; index < code.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, code[index]);
    AnyBox::Shader::NativeFrontend frontend(data);
    auto decoded = std::ranges::find_if(frontend.GetDecoded().instructions, [](const auto& instruction) { return instruction.family == ShaderRecompiler::RdnaInstructionFamily::VOP3 && instruction.opcodeId == 0x140; });
    Fixtures::Require(decoded != frontend.GetDecoded().instructions.end() && decoded->rawWords[0] == fma[0]);
    auto state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
    state.dx10Clamp = true;
    state.ieeeMode = ieee;
    auto target = BufferTests::Target();
    auto translated = AnyBox::Shader::LowerNativeCompute(frontend, state, ShaderRecompiler::ShaderFloatMode{0xC0, true, ieee, false}, target);
    Fixtures::Require(translated.program.Blocks().size() == frontend.GetGraph().blocks.size() + 1);
    Fixtures::Require((translated.program.Metadata().vectorWindowSize != 0) == (window != 0));
    return AnyBox::Shader::CompileNativeCompute(frontend, state, target, {0, 0, 0, 128});
}

std::uint32_t Expected(const std::array<std::uint32_t, 3>& words, unsigned modifier, bool clamp, bool ieee) {
    auto lhs = std::bit_cast<float>(words[0]);
    auto rhs = std::bit_cast<float>(words[1]);
    auto addend = std::bit_cast<float>(words[2]);
    auto value = lhs == 0 || rhs == 0 ? std::fma(0.0f, 0.0f, addend) : std::fma(lhs, rhs, addend);
    constexpr std::array scales{1.0f, 2.0f, 4.0f, 0.5f};
    if (!ieee) value *= scales.at(modifier);
    if (clamp) value = std::isnan(value) || value <= 0 ? 0 : std::min(value, 1.0f);
    return std::bit_cast<std::uint32_t>(value);
}

void Check(AnyBox::Vulkan::Context& context, unsigned modifier, bool clamp, bool ieee, unsigned window) {
    constexpr std::array<std::array<std::uint32_t, 3>, 8> cases{{{0x3F800001, 0x3F7FFFFE, 0xBF800000}, {0x3F800001, 0x3F800001, 0xBF800002}, {0, 0x7F800000, 0x3F000000}, {0xFF800000, 0, 0xBF000000}, {0, 0x7FC12345, 0x3F000000}, {0x7FC12345, 0x80000000, 0xBF000000}, {0x3FC00000, 0x40000000, 0xBF800000}, {0xBFC00000, 0x40000000, 0x3F800000}}};
    auto shader = Compile(modifier, clamp, ieee, window);
    for (const auto& words : cases) {
        std::array<std::uint32_t, 16> input{};
        std::copy(words.begin(), words.end(), input.begin());
        std::array<std::uint32_t, 32> expected;
        expected.fill(Expected(words, modifier, clamp, ieee));
        BufferTests::RunGpu(context, shader, expected, {1, 1, 1}, std::as_bytes(std::span(input)));
    }
}
}

void BufferTests::CheckNativeFma(AnyBox::Vulkan::Context& context) {
    for (unsigned modifier = 0; modifier < 4; ++modifier) {
        for (bool clamp : {false, true}) {
            for (bool ieee : {false, true}) {
                for (unsigned window = 0; window < 6; ++window) Check(context, modifier, clamp, ieee, window);
            }
        }
    }
}
