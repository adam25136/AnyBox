#include "DotKernel.hpp"
#include "FloatFixture.hpp"
#include "Fixtures.hpp"
#include <bit>
#include <cmath>

namespace {
std::uint32_t Expected(std::span<const std::uint32_t> input, unsigned sample, unsigned dimensions, bool precise) {
    auto component = [&](unsigned side, unsigned part) { return std::bit_cast<float>(input[((side == 0 ? sample / 16 : sample) + part * (side == 0 ? 1 : 3)) % 16]); };
    volatile float product = component(0, 0) * component(1, 0);
    float result = product;
    for (unsigned part = 1; part < dimensions; ++part) {
        if (precise) {
            product = component(0, part) * component(1, part);
            result = result + product;
        } else result = std::fma(component(0, part), component(1, part), result);
    }
    return std::isnan(result) ? 0x7FC00000U : std::bit_cast<std::uint32_t>(result);
}

void CheckInvalid(unsigned dimensions) {
    BufferTests::FloatFixture fixture;
    auto name = "dx.op.dot" + std::to_string(dimensions) + ".f32";
    auto arguments = [&] {
        LLVMBC::Vector<LLVMBC::Value*> values{fixture.Integer(32, 52 + dimensions)};
        for (unsigned index = 0; index < dimensions * 2; ++index) values.push_back(fixture.Argument(index));
        return values;
    };
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), {}));
    auto shortCall = arguments();
    shortCall.pop_back();
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), std::move(shortCall)));
    auto wrongOpcode = arguments();
    wrongOpcode[0] = fixture.Integer(32, 51 + dimensions);
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), std::move(wrongOpcode)));
    auto dynamicOpcode = arguments();
    dynamicOpcode[0] = fixture.Argument(0);
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), std::move(dynamicOpcode)));
    fixture.Reject(*fixture.Call(name, fixture.IntegerType(32), arguments()));
    for (unsigned index = 0; index <= dimensions * 2; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(index == 0 ? 16 : 32, 52 + dimensions);
        fixture.Reject(*fixture.Call(name, fixture.FloatType(), std::move(values)));
    }
}
}

void BufferTests::CheckFloatDot(AnyBox::Vulkan::Context& context) {
    std::array<std::uint32_t, 16> input{0xBF800000, 0x3F800001, 0, 0x80000000, 0x3F800000, 0x40000000, 0xC0000000, 0x3F7FFFFE, 0x3F000000, 0xBF000000, 0x7F800000, 0xFF800000, 0x7FC12345, 0x7F812345, 0x477FE000, 0x3EAAAAAB};
    Fixtures::Require(Expected(input, 4, 2, true) == 0 && Expected(input, 4, 2, false) == 0xA8800000);
    for (unsigned dimensions : {2u, 3u, 4u}) {
        CheckInvalid(dimensions);
        for (bool precise : {false, true}) {
            auto program = MakeDotKernel(dimensions, precise);
            ShaderRecompiler::ShaderComputeInputInfo info;
            info.threadsNum[0] = 64;
            info.threadsNum[1] = info.threadsNum[2] = 1;
            info.waveSize = info.hostSubgroupSize = 32;
            auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
            std::vector<std::uint32_t> expected;
            for (unsigned sample = 0; sample < 256; ++sample) expected.push_back(Expected(input, sample, dimensions, precise));
            RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(input)));
        }
    }
}
