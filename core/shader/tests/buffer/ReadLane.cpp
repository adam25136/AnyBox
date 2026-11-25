#include "ReadLaneKernel.hpp"
#include "FloatFixture.hpp"
#include "Fixtures.hpp"
#include "ComputeCompiler.hpp"

void CheckReadLaneControl(AnyBox::Vulkan::Context& context);
void CheckActiveLaneProof();
void CheckIndexedLane(AnyBox::Vulkan::Context& context);

namespace {
void Invalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 51), fixture.Integer(32, 17), fixture.Integer(32, 31)}; };
    auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> args) { fixture.Reject(*fixture.Call("xbox.ReadLane.i32", type, std::move(args))); };
    reject(word, {});
    reject(fixture.FloatType(), arguments());
    auto extra = arguments();
    extra.push_back(fixture.Integer(32, 0));
    reject(word, std::move(extra));
    for (unsigned item = 0; item < 3; ++item) {
        auto args = arguments();
        args[item] = fixture.Integer(16, 0);
        reject(word, std::move(args));
    }
    auto args = arguments();
    args[0] = fixture.Integer(32, 50);
    reject(word, std::move(args));
    args = arguments();
    args[0] = fixture.Make<LLVMBC::Argument>(word, 0, false);
    reject(word, std::move(args));
}

void CheckGpu(AnyBox::Vulkan::Context& context, unsigned wave, unsigned threads, std::span<const std::uint32_t> selectors, unsigned mode = 0) {
    auto program = BufferTests::ReadLaneKernel(wave, selectors, mode);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto local = index % threads;
        auto first = index - local % wave;
        for (auto selector : selectors) {
            auto selected = (selector + (mode == 1 ? index / threads * 33 : 0)) % wave;
            expected.push_back((first + selected) * 17 + 5);
        }
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

namespace BufferTests {
void CheckReadLane(AnyBox::Vulkan::Context& context) {
    Invalid();
    CheckActiveLaneProof();
    CheckIndexedLane(context);
    CheckReadLaneControl(context);
    std::vector<std::uint32_t> selectors;
    for (unsigned lane = 0; lane < 64; ++lane) selectors.push_back(lane);
    for (auto value : {64u, 127u, 128u, UINT32_MAX}) selectors.push_back(value);
    for (unsigned wave : {32u, 64u}) {
        CheckGpu(context, wave, 128, selectors);
        constexpr std::array<std::uint32_t, 4> dynamic{0, 31, 32, UINT32_MAX};
        CheckGpu(context, wave, 128, dynamic, 1);
        constexpr std::array<std::uint32_t, 1> zero{0};
        for (unsigned threads : {1u, 17u, 33u, 65u, 127u}) CheckGpu(context, wave, threads, zero);
        auto program = ReadLaneKernel(wave, zero, 2);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 128;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = wave;
        info.hostSubgroupSize = 32;
        Fixtures::Reject<std::runtime_error>([&] { (void)AnyBox::Shader::Dxil::CompileCompute(program, info, Target(), {0, 0, 0, 128}); });
    }
}
}
