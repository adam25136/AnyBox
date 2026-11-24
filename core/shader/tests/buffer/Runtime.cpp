#include "Fixture.hpp"
#include "Fixtures.hpp"
#include "Target.hpp"
#include "Commands.hpp"
#include "../../../../tools/shader/Compilation.hpp"
#include <anybox/shader/runtime/ComputeBinding.hpp>
#include <cstring>
#include <string_view>

namespace {
AnyBox::Shader::ComputeArtifact MakeArtifact() {
    BufferTests::Fixture fixture(false);
    auto* loaded = fixture.Load(32, 0, 0);
    fixture.WriteOutput(fixture.Extract(loaded, 0));
    auto& program = fixture.Finish();
    ShaderRecompiler::ShaderComputeInputInfo input;
    input.threadsNum[0] = input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    auto result = AnyBox::Shader::CompileCompute(program, input, BufferTests::Target(), {0, 0, 0, 128});
    auto digest = AnyBox::Shader::HashShader(std::as_bytes(std::span(result.code)));
    auto artifact = AnyBox::Shader::Tools::MakeArtifact(std::move(result), {}, input, digest);
    return AnyBox::Shader::DecodeComputeArtifact(AnyBox::Shader::EncodeComputeArtifact(artifact), digest);
}

void Execute(AnyBox::Vulkan::Context& context, AnyBox::Shader::Runtime::ComputeMemory& memory, const AnyBox::Shader::Runtime::ComputeBinding& binding, std::span<const std::byte> constants, bool dispatch) {
    AnyBox::Vulkan::Tests::Commands commands(context);
    auto command = commands.Begin();
    memory.Begin(command);
    binding.Dispatch(command, constants, {dispatch ? 1u : 0u, 1, 1});
    memory.End(command);
    VkMemoryBarrier visible{};
    visible.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    visible.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    visible.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &visible, 0, nullptr, 0, nullptr);
    commands.Submit(command);
}
}

void CheckRuntimeComputeMemory(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox;
    using namespace Shader::Runtime;
    namespace Abi = ShaderRecompiler::BdaAbi;
    auto artifact = MakeArtifact();
    ComputePipeline pipeline(context, artifact);
    Fixtures::Require(pipeline.GetDescriptorMask() == 3 && artifact.userRegisters.empty());
    std::vector<std::byte> constants(artifact.pushConstantSize);
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto input = std::make_shared<Vulkan::Buffer>(Vulkan::Buffer::Allocate(context, 64, usage, properties));
    auto output = std::make_shared<Vulkan::Buffer>(Vulkan::Buffer::Allocate(context, 4, usage, properties));
    Graphics::Buffers::AddressSpace addresses;
    auto source = addresses.Register(0x100000000, 64, input->GetAddress(), input);
    auto destination = addresses.Register(0x200000000, 4, output->GetAddress(), output);
    std::array bindings{MemoryBinding{source, Abi::Read}, MemoryBinding{destination, Abi::Write}};
    ComputeMemory memory(context, MemoryTable(bindings));
    ComputeBinding binding(pipeline, memory);
    auto set = binding.GetSets().front();
    Fixtures::Reject<std::invalid_argument>([&] { memory.Begin(VK_NULL_HANDLE); });
    Fixtures::Reject<std::invalid_argument>([&] { memory.End(VK_NULL_HANDLE); });
    Fixtures::Reject<std::invalid_argument>([&] { binding.Dispatch(VK_NULL_HANDLE, constants, {1, 1, 1}); });
    for (auto expected : {0x12345678u, 0xABCDEF12u}) {
        std::memcpy(input->Map().data(), &expected, sizeof(expected));
        Execute(context, memory, binding, constants, true);
        memory.CheckFault();
        std::uint32_t observed;
        std::memcpy(&observed, output->Map().data(), sizeof(observed));
        Fixtures::Require(observed == expected && binding.GetSets().front() == set);
    }
    for (auto lifetime : {FaultLifetime::Submission, FaultLifetime::Recording}) {
        for (bool permission : {false, true}) {
            std::vector<MemoryBinding> invalid{{destination, Abi::Write}};
            if (permission) invalid.push_back({source, Abi::Write});
            ComputeMemory rejected(context, MemoryTable(invalid), lifetime);
            ComputeBinding selected(pipeline, rejected);
            Execute(context, rejected, selected, constants, true);
            bool caught = false;
            try { rejected.CheckFault(); }
            catch (const std::runtime_error& error) {
                auto reason = permission ? "reason=2 " : "reason=1 ";
                caught = std::string_view(error.what()).find(reason) != std::string_view::npos;
            }
            Fixtures::Require(caught);
            Execute(context, rejected, selected, constants, false);
            if (lifetime == FaultLifetime::Recording) Fixtures::Reject<std::runtime_error>([&] { rejected.CheckFault(); });
            else rejected.CheckFault();
        }
    }
}
