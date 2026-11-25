#include "SharedFixture.hpp"
#include "ComputeCompiler.hpp"
#include "Target.hpp"
#include "Fixtures.hpp"
#include "Commands.hpp"
#include "../../../../tools/shader/Compilation.hpp"
#include <anybox/shader/runtime/ComputeBinding.hpp>
#include <cstring>

void CheckInvocationProof();

void CheckNativeInvocations(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox;
    using namespace Shader::Runtime;
    CheckInvocationProof();
    BufferTests::SharedFixture fixture;
    bool independent = false;
    auto program = fixture.Lower(&independent);
    Fixtures::Require(independent);
    ShaderRecompiler::ShaderComputeInputInfo input;
    input.threadsNum[0] = 64;
    input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    input.ldsSizeDwords = 128;
    auto compiled = Shader::Dxil::CompileCompute(program, input, BufferTests::Target(), {0, 0, 0, 128});
    auto digest = Shader::HashShader(std::as_bytes(std::span(compiled.code)));
    input.waveSize = 64;
    auto original = Shader::Tools::MakeArtifact(std::move(compiled), {}, input, digest);
    original.executionFlags |= Shader::NativeComputeInvocations;
    auto artifact = Shader::DecodeComputeArtifact(Shader::EncodeComputeArtifact(original), digest);
    Fixtures::Require(artifact == original && artifact.waveSize == 64);
    ComputePipeline pipeline(context, artifact);
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto output = std::make_shared<Vulkan::Buffer>(Vulkan::Buffer::Allocate(context, 128 * 12, usage, properties));
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
    for (unsigned index = 0; index < 128; ++index) {
        auto groupBase = index / 64 * 64;
        auto local = index % 64;
        std::array expected{(groupBase + (local ^ 32)) * 17 + 5, ((groupBase + 63 - local) * 17 + 5) ^ 0xA5A5A5A5U, index + 7};
        std::array<std::uint32_t, 3> observed{};
        std::memcpy(observed.data(), output->Map().data() + index * 12, sizeof(observed));
        Fixtures::Require(observed == expected);
    }
    auto mismatched = artifact;
    mismatched.executionFlags = 0;
    Fixtures::Reject<std::invalid_argument>([&] { ComputePipeline rejected(context, mismatched); });
}
