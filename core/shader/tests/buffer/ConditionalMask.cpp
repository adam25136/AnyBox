#include "ConditionalMaskKernel.hpp"
#include "ComputeCompiler.hpp"

void CheckConditionalMaskInvalid();

namespace {
void CheckGpu(AnyBox::Vulkan::Context& context, unsigned wave, unsigned threads, std::span<const std::uint64_t> masks, unsigned representation = 0, bool dynamic = false) {
    auto program = BufferTests::ConditionalMaskKernel(wave, masks, representation, dynamic);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto lane = index % threads % wave;
        for (auto mask : masks) {
            if (representation == 6) mask = 0;
            if (dynamic) mask ^= index | (static_cast<std::uint64_t>(index) << 32);
            bool selected = ((mask >> lane) & 1) != 0;
            expected.push_back(selected ? index ^ 0xA55A8420 : index * 17 + 5);
        }
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

namespace BufferTests {
void CheckConditionalMask(AnyBox::Vulkan::Context& context) {
    CheckConditionalMaskInvalid();
    std::vector<std::uint64_t> masks{0, UINT64_MAX, 0xA55A8420F000000F};
    for (unsigned bit = 0; bit < 64; ++bit) masks.push_back(std::uint64_t{1} << bit);
    for (unsigned wave : {32u, 64u}) {
        CheckGpu(context, wave, 128, masks);
        for (unsigned representation = 1; representation < 7; ++representation) CheckGpu(context, wave, 128, std::span(masks).first(3), representation);
        CheckGpu(context, wave, 128, std::span(masks).first(3), 2, true);
        for (unsigned threads : {1u, 33u, 65u, 127u}) CheckGpu(context, wave, threads, std::span(masks).first(3));
    }
}
}
