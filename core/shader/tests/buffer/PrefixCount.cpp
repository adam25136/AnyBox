#include "BallotKernel.hpp"
#include "FloatFixture.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include <algorithm>
#include <bit>

namespace {
void Invalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 136), fixture.Integer(1, 1)}; };
    auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call("dx.op.wavePrefixOp", type, std::move(values))); };
    reject(word, {});
    auto extra = arguments();
    extra.push_back(fixture.Integer(1, 0));
    reject(word, std::move(extra));
    reject(fixture.FloatType(), arguments());
    reject(fixture.IntegerType(64), arguments());
    for (unsigned index = 0; index < 2; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(16, 0);
        reject(word, std::move(values));
    }
    for (auto opcode : {0u, 135u, 137u}) {
        auto values = arguments();
        values[0] = fixture.Integer(32, opcode);
        reject(word, std::move(values));
    }
    auto values = arguments();
    values[0] = fixture.Make<LLVMBC::Argument>(word, 0, false);
    reject(word, std::move(values));
    values = arguments();
    values[1] = fixture.Integer(32, 1);
    reject(word, std::move(values));
}

void Check(AnyBox::Vulkan::Context& context, unsigned wave, unsigned threads, std::uint64_t mask, std::uint64_t execution = UINT64_MAX, unsigned opcode = 136, std::optional<bool> constantPredicate = {}) {
    auto program = BufferTests::BallotKernel(wave, mask, execution, true, opcode, constantPredicate);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    for (std::size_t offset = 5; offset < shader.code.size(); offset += shader.code[offset] >> 16) Fixtures::Require((shader.code[offset] & 65535) != spv::OpGroupNonUniformShuffle);
    if (constantPredicate) mask = *constantPredicate ? UINT64_MAX : 0;
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto local = index % threads;
        auto lane = local % wave;
        bool active = (execution & (std::uint64_t{1} << (local % 64))) != 0;
        auto selected = mask & execution;
        if (wave == 32) selected = static_cast<std::uint32_t>(selected >> (local % 64 / 32 * 32));
        auto count = opcode == 136 ? lane : std::min(wave, threads - local / wave * wave);
        if (count < 64) selected &= (std::uint64_t{1} << count) - 1;
        expected.push_back(active ? static_cast<std::uint32_t>(std::popcount(selected)) : UINT32_MAX);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

void CheckPrefixBitCount(AnyBox::Vulkan::Context& context) {
    Invalid();
    std::vector<std::uint64_t> masks{0, UINT64_MAX, 0xA55A8420F000000F};
    for (unsigned bit = 0; bit < 64; ++bit) masks.push_back(std::uint64_t{1} << bit);
    for (unsigned wave : {32u, 64u}) {
        for (auto mask : masks) Check(context, wave, 128, mask);
        for (unsigned threads : {1u, 17u, 32u, 33u, 64u, 65u, 95u, 127u}) {
            for (auto mask : {UINT64_MAX, std::uint64_t{0xA55A8420F000000F}}) {
                for (unsigned opcode : {135u, 136u}) Check(context, wave, threads, mask, UINT64_MAX, opcode);
            }
            for (bool predicate : {false, true}) Check(context, wave, threads, 0, UINT64_MAX, 136, predicate);
        }
        Check(context, wave, 128, UINT64_MAX, 0);
    }
    for (auto execution : {std::uint64_t{1}, std::uint64_t{0x8000000080000000}, std::uint64_t{0xAAAAAAAA55555555}, std::uint64_t{0xFFFF00000000FFFF}}) {
        for (auto mask : {std::uint64_t{0}, UINT64_MAX, std::uint64_t{0xA55A8420F000000F}}) Check(context, 32, 128, mask, execution);
        Check(context, 32, 128, 0, execution, 136, true);
    }
}
