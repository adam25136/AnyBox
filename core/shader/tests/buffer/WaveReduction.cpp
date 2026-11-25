#include "WaveKernel.hpp"
#include "Fixtures.hpp"
#include "ComputeCompiler.hpp"
#include <Recompiler.hpp>
#include <bit>
#include <cmath>

void CheckWaveReductionProof(std::span<const std::uint32_t> code);

namespace {
std::uint32_t Expected(std::span<const std::uint32_t> input, std::uint32_t mask, bool maximum) {
    bool found = false;
    std::uint32_t result = 0x7FC00000;
    for (unsigned lane = 0; lane < 32; ++lane) {
        if ((mask & (std::uint32_t{1} << lane)) == 0) continue;
        auto bits = input[lane % input.size()];
        if ((bits & 0x7F800000) == 0) bits &= 0x80000000U;
        auto value = std::bit_cast<float>(bits);
        if (std::isnan(value)) continue;
        if (!found) { result = bits; found = true; continue; }
        if (((result | bits) & 0x7FFFFFFF) == 0) result = maximum ? result & bits : result | bits;
        else result = std::bit_cast<std::uint32_t>(maximum ? std::fmax(std::bit_cast<float>(result), value) : std::fmin(std::bit_cast<float>(result), value));
    }
    return result;
}

void CheckInstructions(std::span<const std::uint32_t> code, bool native) {
    unsigned reductions = 0;
    unsigned shuffles = 0;
    for (std::size_t offset = 5; offset < code.size(); offset += code[offset] >> 16) {
        auto opcode = code[offset] & 65535;
        if (opcode == spv::OpGroupNonUniformUMin || opcode == spv::OpGroupNonUniformUMax) ++reductions;
        if (opcode == spv::OpGroupNonUniformShuffle) ++shuffles;
        if (native && opcode == spv::OpCapability && code[offset + 1] == spv::CapabilityGroupNonUniformShuffle) throw std::runtime_error("Native wave reduction retains an unused shuffle capability");
    }
    if (native && (reductions != 1 || shuffles != 0)) throw std::runtime_error("Native wave min/max did not replace all fallback shuffles: " + std::to_string(reductions) + ", " + std::to_string(shuffles));
    if (!native && (reductions != 0 || shuffles != 32)) throw std::runtime_error("Wave min/max fallback has an unexpected instruction sequence");
}

void Check(AnyBox::Vulkan::Context& context, std::uint32_t mask, unsigned operation, bool native) {
    auto program = BufferTests::MakeWaveKernel(mask, 32, operation, true);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 32;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto target = BufferTests::Target();
    std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    if (native) capabilities.push_back(spv::CapabilityGroupNonUniformArithmetic);
    target.supportedCapabilities = capabilities;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, target, {0, 0, 0, 128});
    CheckInstructions(shader.code, native);
    if (!native && mask == UINT32_MAX) CheckWaveReductionProof(shader.code);
    std::array<std::uint32_t, 16> mixed{0x3F800000, 0xC0000000, 0x3F000000, 0x4B800000, 0, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC12345, 0x7F812345, 1, 0x80000001, 0x007FFFFF, 0x00800000, 0x7F7FFFFF, 0xFF7FFFFF};
    std::array<std::uint32_t, 16> zeros{0, 0x80000000, 1, 0x80000001, 0x007FFFFF, 0x807FFFFF, 0, 0x80000000, 0, 0x80000000, 1, 0x80000001, 0, 0x80000000, 0x007FFFFF, 0x807FFFFF};
    std::array<std::uint32_t, 16> nans;
    std::array<std::uint32_t, 16> negativeZero;
    for (unsigned index = 0; index < nans.size(); ++index) nans[index] = (index & 1) != 0 ? 0xFF812345 : 0x7FC12345;
    negativeZero.fill(0x80000000);
    for (const auto& input : {mixed, zeros, nans, negativeZero}) {
        auto result = Expected(input, mask, operation == 3);
        std::vector<std::uint32_t> expected(64, UINT32_MAX);
        for (unsigned index = 0; index < expected.size(); ++index) {
            if ((mask & (std::uint32_t{1} << (index % 32))) != 0) expected[index] = result;
        }
        BufferTests::RunGpu(context, shader, expected, {2, 1, 1}, std::as_bytes(std::span(input)));
    }
}
}

void BufferTests::CheckWaveReductionGpu(AnyBox::Vulkan::Context& context) {
    CheckWaveReductionInvalid();
    for (unsigned operation : {2u, 3u}) Fixtures::Reject<std::runtime_error>([&] { (void)MakeWaveKernel(UINT64_MAX, 64, operation); });
    for (unsigned operation : {2u, 3u}) {
        for (std::uint32_t mask : {UINT32_MAX, 0xA55A8420U, 0x80000000U, 0xFFFF0000U, 0x00005555U, 1U, 0U}) {
            for (bool native : {false, true}) Check(context, mask, operation, native);
        }
    }
}
