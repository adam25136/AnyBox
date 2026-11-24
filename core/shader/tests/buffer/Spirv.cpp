#include "Fixture.hpp"
#include "Fixtures.hpp"
#include "ComputeCompiler.hpp"
#include "Target.hpp"
#include "TransferFixture.hpp"
#include "FloatFixture.hpp"
#include "LocalFixture.hpp"
#include "ConstantFixture.hpp"
#include <anybox/vulkan/Context.hpp>
#include <spirv/unified1/spirv.hpp>

namespace BufferTests { void CheckGpu(AnyBox::Vulkan::Context& context, const AnyBox::Shader::Dxil::ComputeCompilation& shader, std::uint32_t expected); }
void CheckRuntimeComputeMemory(AnyBox::Vulkan::Context& context);

namespace {
void CheckPackedField(AnyBox::Vulkan::Context& context, unsigned byteOffset, std::uint32_t expected) {
    using namespace ShaderRecompiler;
    BufferTests::Fixture fixture(true);
    auto* load = fixture.Load(16, 1, byteOffset);
    fixture.WriteOutput(fixture.Extract(load, 0));
    auto& program = fixture.Finish();
    auto target = BufferTests::Target();
    ShaderComputeInputInfo input;
    input.threadsNum[0] = input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    auto result = AnyBox::Shader::CompileCompute(program, input, target, {0, 0, 0, 128});
    Fixtures::Require(result.code.size() >= 5 && result.code[0] == 0x07230203);
    bool physicalAddress = false;
    for (std::size_t offset = 5; offset < result.code.size();) {
        auto words = result.code[offset] >> 16;
        auto opcode = result.code[offset] & 65535;
        Fixtures::Require(words != 0 && words <= result.code.size() - offset);
        if (opcode == spv::OpTypeInt || opcode == spv::OpTypeFloat) Fixtures::Require(words >= 3 && result.code[offset + 2] != 16);
        if (opcode == spv::OpCapability) {
            Fixtures::Require(words == 2);
            auto capability = result.code[offset + 1];
            Fixtures::Require(capability != spv::CapabilityInt16 && capability != spv::CapabilityFloat16 && capability != spv::CapabilityStorageBuffer16BitAccess);
            physicalAddress |= capability == spv::CapabilityPhysicalStorageBufferAddresses;
        }
        offset += words;
    }
    Fixtures::Require(physicalAddress && program.Info().usesDma && program.Info().bdaWrites);
    BufferTests::CheckGpu(context, result, expected);
}
}

void BufferTests::CheckSpirv() {
    AnyBox::Vulkan::Context context(0);
    CheckRuntimeComputeMemory(context);
    CheckPackedField(context, 28, 0xBEBB);
    CheckPackedField(context, 30, 0xC4C1);
    CheckThreads(context);
    CheckTransfers(context);
    CheckFloats(context);
    CheckFloatTransfers(context);
    CheckLocals(context);
    CheckSharedGpu(context);
    CheckConstants(context);
    CheckBitFields(context);
    CheckUnaryBitsGpu(context);
    CheckNativeGpu(context);
    CheckNativeFma(context);
    CheckFirstLaneGpu(context);
    CheckBallotLoopGpu(context);
    CheckBufferArgumentGpu(context);
    CheckLegacyHalfGpu(context);
    CheckFloatMinMax(context);
    CheckFloatMinMax3(context);
    CheckFloatMad(context);
    CheckFloatDot(context);
    CheckFloatDivision(context);
    CheckWaveFirstGpu(context);
    CheckWaveCountGpu(context);
    CheckWaveBuiltinsGpu(context);
    CheckWaveIntegerGpu(context);
    CheckWaveReductionGpu(context);
    CheckGlobalFloatAtomicGpu(context);
    CheckGlobalStoreGpu(context);
    CheckBufferAtomicGpu(context);
    CheckUndefGpu(context);
    CheckTypedLoadGpu(context);
}
