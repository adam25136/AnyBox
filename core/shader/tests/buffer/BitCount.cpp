#include "BitCountKernel.hpp"
#include "ComputeCompiler.hpp"
#include <bit>
#include <algorithm>

void CheckBitCountInvalid(bool prefix);

namespace {
void CheckGpu(AnyBox::Vulkan::Context& context, unsigned wave, unsigned threads, std::span<const std::uint64_t> masks, unsigned mode, bool prefix) {
    auto program = BufferTests::BitCountKernel(wave, masks, mode, prefix);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        for (auto mask : masks) {
            if (mode == 1) mask ^= index | (static_cast<std::uint64_t>(index) << 32);
            if (mode == 2) {
                auto local = index % threads;
                if (wave == 32) mask = static_cast<std::uint32_t>(mask >> (local % 64 / 32 * 32));
                auto active = std::min(wave, threads - local / wave * wave);
                if (active < 64) mask &= (std::uint64_t{1} << active) - 1;
            }
            if (prefix) mask &= (std::uint64_t{1} << (index % threads % wave)) - 1;
            expected.push_back(static_cast<std::uint32_t>(std::popcount(mask)));
        }
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}

void CheckAll(AnyBox::Vulkan::Context& context, bool prefix) {
    CheckBitCountInvalid(prefix);
    std::vector<std::uint64_t> masks{0, UINT64_MAX, 0xA55A8420F000000F, 0xFFFFFFFF, 0xFFFFFFFF00000000};
    for (unsigned bit = 0; bit < 64; ++bit) {
        masks.push_back(std::uint64_t{1} << bit);
        masks.push_back(~(std::uint64_t{1} << bit));
    }
    for (unsigned wave : {32u, 64u}) {
        for (unsigned mode = 0; mode < 3; ++mode) {
            for (std::size_t first = 0; first < masks.size(); first += 16) CheckGpu(context, wave, 128, std::span(masks).subspan(first, std::min<std::size_t>(16, masks.size() - first)), mode, prefix);
        }
        for (unsigned threads : {1u, 17u, 33u, 65u, 95u, 127u}) CheckGpu(context, wave, threads, std::span(masks).first(5), 2, prefix);
    }
}
}

namespace BufferTests {
void CheckBitCount64(AnyBox::Vulkan::Context& context) { CheckAll(context, false); }
void CheckMaskBitCount64(AnyBox::Vulkan::Context& context) { CheckAll(context, true); }
}
