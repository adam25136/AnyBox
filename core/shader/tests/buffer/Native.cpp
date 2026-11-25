#include "Target.hpp"
#include "NativeFixtures.hpp"
#include <anybox/shader/NativeCompute.hpp>
#include <Recompiler.hpp>
#include <array>
#include <vector>
#include <bit>

namespace {
void Check(AnyBox::Vulkan::Context& context, unsigned scratchMask) {
    auto data = Fixtures::MakeComputeProgram();
    Fixtures::Set(data, 0x13C, 32);
    Fixtures::Set(data, 0x140, 1);
    Fixtures::Set(data, 0x144, 1);
    constexpr std::array<std::uint32_t, 5> body{0x7E040282, 0x34020082, 0xDC708000, 0x007F0001, 0xBF810000};
    std::vector<std::uint32_t> code;
    code.reserve(body.size() + std::popcount(scratchMask & 3u) * 2);
    for (unsigned index = 0; index < 2; ++index) {
        if ((scratchMask & (1u << index)) != 0) {
            code.push_back(0xBA80F814 + index);
            code.push_back(0);
        }
    }
    code.insert(code.end(), body.begin(), body.end());
    auto bytes = (40 + code.size() * 4 + 15) & ~std::size_t{15};
    data.resize(0x140 + bytes);
    Fixtures::Set(data, 0xC8, static_cast<std::uint32_t>(bytes));
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    for (std::size_t index = 0; index < code.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, code[index]);
    AnyBox::Shader::NativeFrontend frontend(data);
    auto state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
    auto target = BufferTests::Target();
    auto compiled = AnyBox::Shader::CompileNativeCompute(frontend, state, target, {0, 0, 0, 128});
    std::array<std::uint32_t, 32> expected{};
    for (unsigned index = 0; index < expected.size(); ++index) expected[index] = index;
    BufferTests::RunGpu(context, compiled, expected, {1, 1, 1});
}
}

void BufferTests::CheckNativeGpu(AnyBox::Vulkan::Context& context) {
    for (unsigned mask = 0; mask < 4; ++mask) Check(context, mask);
}
