#include "SpannedControlKernel.hpp"
#include "WaveControlKernel.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"
#include "Commands.hpp"
#include "../../../../tools/shader/Compilation.hpp"
#include <anybox/shader/runtime/ComputeBinding.hpp>
#include <cstring>

namespace {
void Check(AnyBox::Vulkan::Context& context, bool prefix) {
    using namespace AnyBox;
    using namespace Shader::Runtime;
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = 64;
    info.hostSubgroupSize = 32;
    auto target = BufferTests::Target();
    target.spannedWaves = target.spannedAnySubgroup = target.spannedShuffleThroughMemory = true;
    auto rejected = BufferTests::MakeSpannedControlKernel(prefix);
    Fixtures::Reject<std::runtime_error>([&] { (void)Shader::Dxil::CompileCompute(rejected, info, BufferTests::Target(), {0, 0, 0, 128}); });
    auto program = BufferTests::MakeSpannedControlKernel(prefix);
    auto compiled = Shader::Dxil::CompileCompute(program, info, target, {0, 0, 0, 128});
    Fixtures::Require(compiled.info.fullSubgroups && compiled.info.anySubgroupSize);
    auto digest = Shader::HashShader(std::as_bytes(std::span(compiled.code)));
    auto original = Shader::Tools::MakeArtifact(std::move(compiled), {}, info, digest);
    auto artifact = Shader::DecodeComputeArtifact(Shader::EncodeComputeArtifact(original), digest);
    Fixtures::Require(artifact == original && artifact.executionFlags == Shader::SpannedComputeWave);
    ComputePipeline pipeline(context, artifact);
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto output = std::make_shared<Vulkan::Buffer>(Vulkan::Buffer::Allocate(context, 128 * 4, usage, properties));
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
        auto lane = index % 64;
        auto base = index - lane;
        auto limit = (lane & 3) + (lane < 32 ? 1 : 4);
        auto expected = prefix ? lane * base + lane * (lane + 1) / 2 : base * 64 + 2080;
        expected += limit * index + limit * (limit - 1) / 2;
        std::uint32_t observed;
        std::memcpy(&observed, output->Map().data() + index * 4, sizeof(observed));
        Fixtures::Require(observed == expected);
    }
    auto mismatch = artifact;
    mismatch.executionFlags = 0;
    Fixtures::Reject<std::invalid_argument>([&] { ComputePipeline invalid(context, mismatch); });
    mismatch = artifact;
    mismatch.executionFlags |= Shader::NativeComputeInvocations;
    Fixtures::Reject<std::invalid_argument>([&] { Shader::EncodeComputeArtifact(mismatch); });
    mismatch = artifact;
    mismatch.threads[0] = 128;
    Fixtures::Reject<std::invalid_argument>([&] { Shader::EncodeComputeArtifact(mismatch); });
    auto unsafe = BufferTests::WaveControlKernel(4);
    bool barrierRejected = false;
    try { (void)Shader::Dxil::CompileCompute(unsafe, info, target, {0, 0, 0, 128}); }
    catch (const std::exception& error) { barrierRejected = std::string_view(error.what()).find("OpControlBarrier") != std::string_view::npos; }
    Fixtures::Require(barrierRejected);
}
}

void CheckSpannedRuntime(AnyBox::Vulkan::Context& context) {
    Check(context, false);
    Check(context, true);
}
