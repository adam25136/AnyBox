#include "BallotKernel.hpp"
#include "FloatFixture.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include <algorithm>

namespace {
void Invalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto* type = LLVMBC::StructType::get(word->getContext(), LLVMBC::Vector<LLVMBC::Type*>{word, word}, "ballot");
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 25), fixture.Integer(32, 1)}; };
    fixture.Reject(*fixture.Call("xbox.Ballot64", type, {}));
    fixture.Reject(*fixture.Call("xbox.Ballot64", word, arguments()));
    fixture.Reject(*fixture.Call("xbox.Ballot64", fixture.IntegerType(64), arguments()));
    fixture.Reject(*fixture.Call("xbox.Ballot64", type, {fixture.Integer(32, 25), fixture.Integer(1, 1)}));
    for (auto members : {LLVMBC::Vector<LLVMBC::Type*>{word}, LLVMBC::Vector<LLVMBC::Type*>{word, word, word}, LLVMBC::Vector<LLVMBC::Type*>{word, fixture.FloatType()}}) {
        auto* invalid = LLVMBC::StructType::get(word->getContext(), std::move(members), "invalid");
        fixture.Reject(*fixture.Call("xbox.Ballot64", invalid, arguments()));
    }
    auto wrong = arguments();
    wrong[0] = fixture.Integer(32, 26);
    fixture.Reject(*fixture.Call("xbox.Ballot64", type, std::move(wrong)));
    auto dynamic = arguments();
    dynamic[0] = fixture.Make<LLVMBC::Argument>(word, 5, false);
    fixture.Reject(*fixture.Call("xbox.Ballot64", type, std::move(dynamic)));
    for (unsigned index = 0; index < 2; ++index) {
        auto narrow = arguments();
        narrow[index] = fixture.Integer(16, 25);
        fixture.Reject(*fixture.Call("xbox.Ballot64", type, std::move(narrow)));
    }
}

void CheckGpu(AnyBox::Vulkan::Context& context, unsigned waveSize, std::uint64_t mask, std::uint64_t execution = UINT64_MAX) {
    auto program = BufferTests::BallotKernel(waveSize, mask, execution);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = waveSize;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < 256; ++index) {
        bool active = (execution & (std::uint64_t{1} << (index % 64))) != 0;
        auto ballot = mask & execution;
        if (waveSize == 32) ballot = static_cast<std::uint32_t>(ballot >> ((index % 64) / 32 * 32));
        expected.push_back(active ? static_cast<std::uint32_t>(ballot) : UINT32_MAX);
        expected.push_back(active ? static_cast<std::uint32_t>(ballot >> 32) : UINT32_MAX);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}

void CheckPartial(AnyBox::Vulkan::Context& context, unsigned threads) {
    auto program = BufferTests::BallotKernel(64, UINT64_MAX);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto waveStart = index % threads / 64 * 64;
        auto active = std::min(64u, threads - waveStart);
        auto ballot = active == 64 ? UINT64_MAX : (std::uint64_t{1} << active) - 1;
        expected.push_back(static_cast<std::uint32_t>(ballot));
        expected.push_back(static_cast<std::uint32_t>(ballot >> 32));
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

namespace BufferTests {
void CheckBallot64(AnyBox::Vulkan::Context& context) {
    Invalid();
    std::vector<std::uint64_t> masks{0, UINT64_MAX, 0xA55A8420F000000F};
    for (unsigned lane = 0; lane < 64; ++lane) masks.push_back(std::uint64_t{1} << lane);
    for (unsigned wave : {32u, 64u}) {
        for (auto mask : masks) CheckGpu(context, wave, mask);
    }
    for (std::uint64_t execution : {0ULL, 1ULL, 0x8000000080000000ULL, 0xAAAAAAAA55555555ULL, 0xFFFF00000000FFFFULL}) {
        for (std::uint64_t mask : {0ULL, UINT64_MAX, 0xA55A8420F000000FULL}) CheckGpu(context, 32, mask, execution);
    }
    for (unsigned threads : {1u, 33u, 65u, 95u, 127u, 128u}) CheckPartial(context, threads);
}
}
