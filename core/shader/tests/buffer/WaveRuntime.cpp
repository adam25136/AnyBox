#include "WaveControlKernel.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include "Commands.hpp"
#include "../../../../tools/shader/Compilation.hpp"
#include <anybox/shader/runtime/ComputeBinding.hpp>
#include <cstring>

namespace BufferTests {
void CheckWave64Runtime(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox;
    using namespace Shader::Runtime;
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    for (unsigned mode : {2u, 3u}) {
        auto program = WaveControlKernel(mode);
        auto compiled = Shader::Dxil::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        auto digest = Shader::HashShader(std::as_bytes(std::span(compiled.code)));
        auto original = Shader::Tools::MakeArtifact(std::move(compiled), {}, info, digest);
        auto artifact = Shader::DecodeComputeArtifact(Shader::EncodeComputeArtifact(original), digest);
        Fixtures::Require(artifact.waveSize == 64 && artifact.hostSubgroupSize == 32 && artifact == original);
        ComputePipeline pipeline(context, artifact);
        constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        auto output = std::make_shared<Vulkan::Buffer>(Vulkan::Buffer::Allocate(context, 256 * 4, usage, properties));
        Graphics::Buffers::AddressSpace addresses;
        auto registered = addresses.Register(0x200000000, output->GetSize(), output->GetAddress(), output);
        std::array entries{MemoryBinding{registered, ShaderRecompiler::BdaAbi::Write}};
        ComputeMemory memory(context, MemoryTable(entries));
        ComputeBinding binding(pipeline, memory);
        std::vector<std::byte> constants(artifact.pushConstantSize);
        Vulkan::Tests::Commands commands(context);
        auto command = commands.Begin();
        memory.Begin(command);
        binding.Dispatch(command, constants, {2, 1, 1});
        memory.End(command);
        VkMemoryBarrier visible{};
        visible.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        visible.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        visible.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &visible, 0, nullptr, 0, nullptr);
        commands.Submit(command);
        memory.CheckFault();
        for (unsigned index = 0; index < 256; ++index) {
            std::uint32_t observed;
            std::memcpy(&observed, output->Map().data() + index * 4, sizeof(observed));
            bool active = mode == 2 ? (index / 128) % 2 != 0 : (index & 64) != 0;
            auto expected = active ? index + index / 64 * 64 : index ^ 0xA55A8420;
            Fixtures::Require(observed == expected);
        }
    }
}
}
