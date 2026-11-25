#include "PredicationKernel.hpp"
#include "flow/predication/Plan.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include <bit>

namespace {
std::uint32_t InputWord(unsigned index) {
    std::uint32_t result = 0;
    for (unsigned byte = 0; byte < 4; ++byte) result |= (((index % 16 * 4 + byte) * 3 + 7) & 255) << (byte * 8);
    return result;
}

void CheckGpu(AnyBox::Vulkan::Context& context, unsigned mode, unsigned branches, unsigned threads, bool load, bool diagnostic, unsigned phiKind = 0, bool bitCount = false) {
    auto program = BufferTests::PredicationKernel(mode, branches, load, diagnostic, phiKind, bitCount);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    Fixtures::Require(AnyBox::Shader::Dxil::Flow::Predication::Run(program, info) == (mode == 3 ? 0 : 1));
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto lane = index % threads % 64;
        bool active = mode == 0 ? (index & 1) != 0 : mode == 1 ? lane == 63 : mode == 2 ? lane < 32 : index / threads != 0;
        bool writes = active ? branches != 1 : branches != 0;
        auto payload = load ? InputWord(index) + index : index * 17 + 5;
        if (!active) payload ^= 0xF0C33CAB;
        if (bitCount) payload = static_cast<std::uint32_t>(std::popcount(payload) + std::popcount(lane));
        expected.push_back(writes ? payload : UINT32_MAX);
        auto selected = writes ? payload : 0xDEADBEEF;
        expected.push_back(phiKind == 1 ? selected & 1 : selected);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

namespace BufferTests {
void CheckPredication(AnyBox::Vulkan::Context& context) {
    for (unsigned mode = 0; mode < 4; ++mode) {
        for (unsigned branches = 0; branches < 3; ++branches) CheckGpu(context, mode, branches, 128, false, false);
    }
    for (unsigned mode = 0; mode < 3; ++mode) {
        for (unsigned branches = 0; branches < 3; ++branches) CheckGpu(context, mode, branches, 128, true, true);
    }
    for (unsigned phiKind : {1u, 2u}) CheckGpu(context, 0, 2, 128, true, true, phiKind);
    for (unsigned mode : {0u, 1u}) CheckGpu(context, mode, 2, 128, true, true, 0, true);
    for (unsigned threads : {1u, 33u, 65u, 95u, 127u}) CheckGpu(context, 1, 2, threads, true, true);
    auto program = PredicationKernel(1, 0, true, 2);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected(512);
    bool fault = false;
    try { RunGpu(context, shader, expected, {2, 1, 1}); }
    catch (const std::runtime_error& error) { fault = std::string_view(error.what()).starts_with("DXIL BDA GPU test recorded fault "); }
    Fixtures::Require(fault);
}
}
