#include "SwizzleKernel.hpp"
#include "ComputeCompiler.hpp"
#include "FloatFixture.hpp"
#include "Fixtures.hpp"

namespace {
void Invalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 53), fixture.Integer(32, 17), fixture.Integer(32, 30)}; };
    auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call("xbox.LaneSwizzle.i32", type, std::move(values))); };
    reject(word, {});
    reject(fixture.FloatType(), arguments());
    auto extra = arguments();
    extra.push_back(fixture.Integer(32, 0));
    reject(word, std::move(extra));
    for (unsigned item = 0; item < 3; ++item) {
        auto values = arguments();
        values[item] = fixture.Integer(16, 0);
        reject(word, std::move(values));
    }
    for (unsigned item : {0u, 2u}) {
        auto values = arguments();
        values[item] = fixture.Make<LLVMBC::Argument>(word, 1, false);
        reject(word, std::move(values));
    }
    for (std::uint32_t pattern : {0xC000u, 0xE000u, 0x10000u, UINT32_MAX}) {
        auto values = arguments();
        values[2] = fixture.Integer(32, pattern);
        reject(word, std::move(values));
    }
    auto values = arguments();
    values[0] = fixture.Integer(32, 52);
    reject(word, std::move(values));
}

unsigned SourceLane(std::uint32_t pattern, unsigned lane) {
    if (pattern & 0x8000) return (lane & ~3u) | ((pattern >> ((lane & 3) * 2)) & 3);
    return (lane & ~31u) | (((lane & (pattern & 31)) | ((pattern >> 5) & 31)) ^ ((pattern >> 10) & 31));
}

void CheckGpu(AnyBox::Vulkan::Context& context, unsigned wave, unsigned threads, std::span<const std::uint32_t> patterns, unsigned first = 0) {
    auto program = BufferTests::SwizzleKernel(wave, patterns, first);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto local = index % threads;
        auto lane = local % wave;
        for (auto pattern : patterns) {
            auto source = SourceLane(pattern, lane);
            auto groupSource = local - lane + source;
            auto value = groupSource >= threads || source < first ? 0 : (index - local + groupSource) * 17 + 5;
            expected.push_back(lane < first ? UINT32_MAX : value);
        }
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

namespace BufferTests {
void CheckLaneSwizzle(AnyBox::Vulkan::Context& context) {
    Invalid();
    std::vector<std::uint32_t> patterns{0, 31, 30, 60, 120, 240, 480, 31 | (31 << 10), 31 << 5, 0x7FFF, 0x1248, 0x552A};
    for (unsigned quad = 0; quad < 256; ++quad) patterns.push_back(0x8000 | quad);
    for (unsigned wave : {32u, 64u}) CheckGpu(context, wave, 128, patterns);
    constexpr std::array<std::uint32_t, 7> subset{0, 31, 31 | (31 << 10), 0x8000, 0x8055, 0x80AA, 0x80FF};
    for (unsigned wave : {32u, 64u}) for (unsigned threads : {1u, 17u, 33u, 65u, 95u, 127u}) CheckGpu(context, wave, threads, subset);
    for (unsigned first : {1u, 7u, 16u, 31u}) CheckGpu(context, 32, 128, subset, first);
}
}
