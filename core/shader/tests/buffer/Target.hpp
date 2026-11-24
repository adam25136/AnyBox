#pragma once

#include <anybox/shader/ComputeCompiler.hpp>
#include <spirv/unified1/spirv.hpp>

namespace AnyBox::Vulkan { class Context; }

namespace BufferTests {
inline ShaderRecompiler::SpirvTargetOptions Target() {
    static constexpr std::array<std::uint32_t, 5> capabilities{spv::CapabilityShader, spv::CapabilityInt64, spv::CapabilityGroupNonUniform, spv::CapabilityPhysicalStorageBufferAddresses, spv::CapabilityStorageBuffer8BitAccess};
    static constexpr std::array<std::string_view, 3> extensions{"SPV_KHR_storage_buffer_storage_class", "SPV_KHR_physical_storage_buffer", "SPV_KHR_8bit_storage"};
    ShaderRecompiler::SpirvTargetOptions target{};
    target.vulkanVersion = 0x00402000;
    target.spirvVersion = 0x00010500;
    target.subgroupSize = 32;
    target.bdaAbiVersion = 1;
    target.supportedCapabilities = capabilities;
    target.supportedExtensions = extensions;
    return target;
}

void RunGpu(AnyBox::Vulkan::Context& context, const AnyBox::Shader::ComputeCompilation& shader, std::span<const std::uint32_t> expected, const std::array<std::uint32_t, 3>& groups, std::span<const std::byte> inputData = {}, std::span<const std::uint32_t> userData = {}, std::span<const std::byte> outputData = {});
void CheckGlobalFloatAtomicGpu(AnyBox::Vulkan::Context& context);
void CheckGlobalStoreGpu(AnyBox::Vulkan::Context& context);
void CheckUndefGpu(AnyBox::Vulkan::Context& context);
void CheckTypedLoadGpu(AnyBox::Vulkan::Context& context);
void CheckThreads(AnyBox::Vulkan::Context& context);
void CheckNativeGpu(AnyBox::Vulkan::Context& context);
void CheckNativeFma(AnyBox::Vulkan::Context& context);
void CheckFirstLaneGpu(AnyBox::Vulkan::Context& context);
void CheckBallotLoopGpu(AnyBox::Vulkan::Context& context);
void CheckBufferArgumentGpu(AnyBox::Vulkan::Context& context);
void CheckLegacyHalfGpu(AnyBox::Vulkan::Context& context);
void CheckFloatMinMax(AnyBox::Vulkan::Context& context);
void CheckFloatMad(AnyBox::Vulkan::Context& context);
void CheckWaveFirstGpu(AnyBox::Vulkan::Context& context);
void CheckWaveCountGpu(AnyBox::Vulkan::Context& context);
void CheckWaveBuiltinsGpu(AnyBox::Vulkan::Context& context);
void CheckUnaryBitsGpu(AnyBox::Vulkan::Context& context);
void CheckWaveIntegerGpu(AnyBox::Vulkan::Context& context);
void CheckSharedGpu(AnyBox::Vulkan::Context& context);
void CheckWaveReductionGpu(AnyBox::Vulkan::Context& context);
void CheckWaveReductionInvalid();
}
