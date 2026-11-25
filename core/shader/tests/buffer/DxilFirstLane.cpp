#include "UniformKernel.hpp"
#include "FloatFixture.hpp"
#include "ComputeCompiler.hpp"

namespace {
void Invalid(unsigned bits, bool floating) {
    BufferTests::FloatFixture fixture;
    auto* type = floating ? fixture.FloatType() : fixture.IntegerType(bits);
    auto* name = BufferTests::FirstLaneName(bits, floating);
    auto input = [&]() -> LLVMBC::Value* { return floating ? static_cast<LLVMBC::Value*>(fixture.FloatConstant(0x80000000)) : fixture.Integer(bits, 1); };
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 118), input()}; };
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call(name, result, std::move(values))); };
    reject(type, {});
    auto extra = arguments();
    extra.push_back(input());
    reject(type, std::move(extra));
    reject(fixture.IntegerType(16), arguments());
    for (auto* opcode : std::array<LLVMBC::Value*, 3>{fixture.Integer(16, 118), fixture.Integer(32, 119), fixture.Make<LLVMBC::Argument>(fixture.IntegerType(32), 0, false)}) {
        auto values = arguments();
        values[0] = opcode;
        reject(type, std::move(values));
    }
    auto values = arguments();
    values[1] = fixture.Integer(16, 0);
    reject(type, std::move(values));
    if (bits == 32) {
        values = arguments();
        values[1] = floating ? static_cast<LLVMBC::Value*>(fixture.Integer(32, 0)) : fixture.FloatConstant(0);
        reject(type, std::move(values));
    }
}

void CheckGpu(AnyBox::Vulkan::Context& context, unsigned bits, bool floating, unsigned wave, unsigned threads, unsigned first, bool uniform) {
    auto program = BufferTests::UniformKernel(bits, wave, first, uniform, true, floating);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = threads;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = wave;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < threads * 2; ++index) {
        auto lane = index % threads % wave;
        bool active = lane >= first;
        auto selected = index - lane + first;
        auto value = uniform ? bits == 1 ? 1u : 0xA55AC33C : bits == 1 ? static_cast<unsigned>((selected & 65) != 0) : selected * 17 + 5;
        expected.push_back(active ? value : 0xDEADBEEF);
        if (bits == 64) expected.push_back(!active ? 0xCAFEBABE : uniform ? 0x81FEDCBA : selected ^ 0xA5F03C69);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

namespace BufferTests {
void CheckDxilFirstLane(AnyBox::Vulkan::Context& context) {
    for (auto overload : {std::pair{1u, false}, {32u, false}, {64u, false}, {32u, true}}) {
        auto [bits, floating] = overload;
        Invalid(bits, floating);
        for (bool uniform : {false, true}) {
            for (unsigned first : {0u, 1u, 7u, 16u, 31u, 32u}) CheckGpu(context, bits, floating, 32, 128, first, uniform);
            CheckGpu(context, bits, floating, 64, 128, 0, uniform);
        }
        for (unsigned threads : {1u, 33u, 65u, 127u}) CheckGpu(context, bits, floating, 64, threads, 0, false);
    }
}
}
