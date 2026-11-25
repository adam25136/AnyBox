#include "UniformKernel.hpp"
#include "FloatFixture.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

namespace {
void Invalid(unsigned bits) {
    BufferTests::FloatFixture fixture;
    auto* type = fixture.IntegerType(bits);
    auto* name = BufferTests::UniformName(bits);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 50), fixture.Integer(bits, 123)}; };
    fixture.Reject(*fixture.Call(name, type, {}));
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), arguments()));
    auto extra = arguments();
    extra.push_back(fixture.Integer(bits, 0));
    fixture.Reject(*fixture.Call(name, type, std::move(extra)));
    auto wrong = arguments();
    wrong[0] = fixture.Integer(32, 51);
    fixture.Reject(*fixture.Call(name, type, std::move(wrong)));
    auto dynamic = arguments();
    dynamic[0] = fixture.Make<LLVMBC::Argument>(fixture.IntegerType(32), 5, false);
    fixture.Reject(*fixture.Call(name, type, std::move(dynamic)));
    for (unsigned index = 0; index < 2; ++index) {
        auto narrow = arguments();
        narrow[index] = fixture.Integer(16, 50);
        fixture.Reject(*fixture.Call(name, type, std::move(narrow)));
    }
}

void CheckGpu(AnyBox::Vulkan::Context& context, unsigned bits, unsigned waveSize, unsigned first, bool uniform) {
    auto program = BufferTests::UniformKernel(bits, waveSize, first, uniform);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = waveSize;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    unsigned broadcasts = 0;
    for (std::size_t offset = 5; offset < shader.code.size();) {
        auto length = shader.code[offset] >> 16;
        Fixtures::Require(length != 0 && length <= shader.code.size() - offset);
        if ((shader.code[offset] & 65535) == spv::OpGroupNonUniformBroadcastFirst) ++broadcasts;
        offset += length;
    }
    if (waveSize == 32) Fixtures::Require(broadcasts == (uniform ? 0 : bits / 32));
    std::vector<std::uint32_t> expected;
    for (unsigned index = 0; index < 256; ++index) {
        bool active = index % waveSize >= first;
        auto selected = index - index % waveSize + first;
        expected.push_back(!active ? 0xDEADBEEF : uniform ? 0xA55AC33C : selected * 17 + 5);
        if (bits == 64) expected.push_back(!active ? 0xCAFEBABE : uniform ? 0x81FEDCBA : selected ^ 0xA5F03C69);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

namespace BufferTests {
void CheckMakeUniform(AnyBox::Vulkan::Context& context) {
    for (unsigned bits : {32u, 64u}) {
        Invalid(bits);
        for (bool uniform : {false, true}) {
            for (unsigned first : {0u, 1u, 17u, 31u, 32u}) CheckGpu(context, bits, 32, first, uniform);
            CheckGpu(context, bits, 64, 0, uniform);
        }
    }
}
}
