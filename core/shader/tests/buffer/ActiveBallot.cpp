#include "BallotKernel.hpp"
#include "FloatFixture.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include <algorithm>

namespace {
void Invalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto* result = LLVMBC::StructType::get(word->getContext(), LLVMBC::Vector<LLVMBC::Type*>(4, word), "ballot");
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 116), fixture.Integer(1, 1)}; };
    auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call("dx.op.waveActiveBallot", type, std::move(values))); };
    reject(result, {});
    reject(word, arguments());
    auto extra = arguments();
    extra.push_back(fixture.Integer(1, 0));
    reject(result, std::move(extra));
    for (unsigned words : {1u, 2u, 3u, 5u}) reject(LLVMBC::StructType::get(word->getContext(), LLVMBC::Vector<LLVMBC::Type*>(words, word), "invalid"), arguments());
    for (unsigned member = 0; member < 4; ++member) {
        LLVMBC::Vector<LLVMBC::Type*> members(4, word);
        members[member] = fixture.FloatType();
        reject(LLVMBC::StructType::get(word->getContext(), std::move(members), "invalid"), arguments());
    }
    for (unsigned index = 0; index < 2; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(16, 0);
        reject(result, std::move(values));
    }
    auto values = arguments();
    values[1] = fixture.Integer(32, 1);
    reject(result, std::move(values));
    values = arguments();
    values[0] = fixture.Integer(32, 117);
    reject(result, std::move(values));
    values = arguments();
    values[0] = fixture.Make<LLVMBC::Argument>(word, 0, false);
    reject(result, std::move(values));
}

void Check(AnyBox::Vulkan::Context& context, unsigned wave, unsigned threads, std::uint64_t mask, std::uint64_t execution = UINT64_MAX) {
    auto program = BufferTests::BallotKernel(wave, mask, execution, true);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto local = index % threads;
        bool active = (execution & (std::uint64_t{1} << (local % 64))) != 0;
        auto ballot = mask & execution;
        if (wave == 32) ballot = static_cast<std::uint32_t>(ballot >> (local % 64 / 32 * 32));
        auto available = std::min(wave, threads - local / wave * wave);
        if (available < 64) ballot &= (std::uint64_t{1} << available) - 1;
        expected.push_back(active ? static_cast<std::uint32_t>(ballot) : UINT32_MAX);
        expected.push_back(active ? static_cast<std::uint32_t>(ballot >> 32) : UINT32_MAX);
        expected.push_back(active ? 0 : UINT32_MAX);
        expected.push_back(active ? 0 : UINT32_MAX);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

void CheckActiveBallot(AnyBox::Vulkan::Context& context) {
    Invalid();
    std::vector<std::uint64_t> masks{0, UINT64_MAX, 0xA55A8420F000000F};
    for (unsigned bit = 0; bit < 64; ++bit) masks.push_back(std::uint64_t{1} << bit);
    for (unsigned wave : {32u, 64u}) {
        for (auto mask : masks) Check(context, wave, 128, mask);
        for (unsigned threads : {1u, 17u, 33u, 65u, 95u, 127u}) {
            for (auto mask : {UINT64_MAX, std::uint64_t{0xA55A8420F000000F}}) Check(context, wave, threads, mask);
        }
        Check(context, wave, 128, UINT64_MAX, 0);
    }
    for (auto execution : {std::uint64_t{1}, std::uint64_t{0x8000000080000000}, std::uint64_t{0xAAAAAAAA55555555}, std::uint64_t{0xFFFF00000000FFFF}}) {
        for (auto mask : {std::uint64_t{0}, UINT64_MAX, std::uint64_t{0xA55A8420F000000F}}) Check(context, 32, 128, mask, execution);
    }
    auto divergent = BufferTests::BallotKernel(64, UINT64_MAX, 0xAAAAAAAA55555555, true);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    Fixtures::Reject<std::runtime_error>([&] { (void)AnyBox::Shader::Dxil::CompileCompute(divergent, info, BufferTests::Target(), {0, 0, 0, 128}); });
}
