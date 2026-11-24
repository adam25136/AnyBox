#include "FloatFixture.hpp"
#include "FloatCallKernel.hpp"
#include "FloatMinMaxReference.hpp"

namespace {
void CheckInvalid(unsigned mode) {
    BufferTests::FloatFixture fixture;
    auto* name = mode == 8 ? "xbox.Min3_F32" : mode == 9 ? "xbox.Max3_F32" : "xbox.Med3_F32";
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, mode), fixture.Argument(0), fixture.Argument(1), fixture.Argument(2)}; };
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), {}));
    fixture.Reject(*fixture.Call(name, fixture.IntegerType(32), arguments()));
    auto shortCall = arguments();
    shortCall.pop_back();
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), std::move(shortCall)));
    auto wrongOpcode = arguments();
    wrongOpcode[0] = fixture.Integer(32, mode + 1);
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), std::move(wrongOpcode)));
    auto dynamic = arguments();
    dynamic[0] = fixture.Argument(0);
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), std::move(dynamic)));
    for (unsigned index = 0; index < 4; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(index == 0 ? 16 : 32, mode);
        fixture.Reject(*fixture.Call(name, fixture.FloatType(), std::move(values)));
    }
}
}

void BufferTests::CheckFloatMinMax3(AnyBox::Vulkan::Context& context) {
    std::array<std::uint32_t, 16> input{0x3F800000, 0xC0000000, 0x3F000000, 0x4B800000, 0, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC12345, 0x7F812345, 1, 0x80000001, 0x007FFFFF, 0x807FFFFF, 0x00800000, 0x7F7FFFFF};
    for (unsigned mode : {8u, 9u, 10u}) {
        CheckInvalid(mode);
        auto* name = mode == 8 ? "xbox.Min3_F32" : mode == 9 ? "xbox.Max3_F32" : "xbox.Med3_F32";
        auto program = MakeFloatCallKernel(mode, 3, name);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 64;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (auto first : input) {
            for (auto second : input) {
                for (auto third : input) expected.push_back(mode == 10 ? Reference::Median(first, second, third) : Reference::MinMax(Reference::MinMax(first, second, mode == 9), third, mode == 9));
            }
        }
        RunGpu(context, shader, expected, {64, 1, 1}, std::as_bytes(std::span(input)));
    }
}
