#include <anybox/shader/runtime/ComputeBinding.hpp>
#include "Commands.hpp"
#include <cstring>

void CheckComputeBindings(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox;
    Shader::Runtime::ComputeMemory memory(context, Shader::Runtime::MemoryTable({}));
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto output = Vulkan::Buffer::Allocate(context, 4, usage, properties);
    auto address = output.GetAddress();
    std::array constants{static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32)};
    for (unsigned mask = 0; mask < 4; ++mask) {
        std::string source = "#version 450\n#extension GL_EXT_buffer_reference : require\nlayout(local_size_x=1) in;\n";
        source += "layout(buffer_reference, std430, buffer_reference_align=4) buffer Output { uint value; };\nlayout(push_constant) uniform Parameters { Output outputBuffer; } args;\n";
        if (mask & 1) source += "layout(set=0,binding=59,std430) readonly buffer Pages { uint words[]; } pages;\n";
        if (mask & 2) source += "layout(set=0,binding=60,std430) readonly buffer Faults { uint words[]; } faults;\n";
        source += "void main() { args.outputBuffer.value = ";
        source += (mask & 1) ? "pages.words[0]" : "5u";
        source += " + ";
        source += (mask & 2) ? "faults.words[0]" : "7u";
        source += "; }\n";
        Shader::ComputeArtifact artifact;
        artifact.sourceHash = Shader::HashShader(std::as_bytes(std::span(source.data(), source.size())));
        artifact.threads = {1, 1, 1};
        artifact.pushConstantSize = sizeof(constants);
        artifact.code = Vulkan::Tests::CompileCompute(source.c_str());
        for (unsigned index = 0; index < 2; ++index) if (mask & (1u << index)) artifact.descriptors.push_back({59 + index, {}});
        Shader::Runtime::ComputePipeline pipeline(context, artifact);
        Shader::Runtime::ComputeBinding binding(pipeline, memory);
        if (pipeline.GetDescriptorMask() != mask || binding.GetSets().size() != (mask ? 1u : 0u)) throw std::runtime_error("Compute descriptor set does not match its artifact binding mask");
        auto retainedSet = binding.GetSets().empty() ? VK_NULL_HANDLE : binding.GetSets().front();
        for (unsigned repeat = 0; repeat < 2; ++repeat) {
            Vulkan::Tests::Commands commands(context);
            auto command = commands.Begin();
            memory.Begin(command);
            binding.Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1});
            memory.End(command);
            VkMemoryBarrier visible{};
            visible.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            visible.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            visible.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &visible, 0, nullptr, 0, nullptr);
            commands.Submit(command);
            memory.CheckFault();
            std::uint32_t value;
            std::memcpy(&value, output.Map().data(), sizeof(value));
            if (value != ((mask & 1) ? 1u : 5u) + ((mask & 2) ? 0u : 7u)) throw std::runtime_error("Compute metadata descriptor resolved the wrong GPU buffer");
            if (!binding.GetSets().empty() && binding.GetSets().front() != retainedSet) throw std::runtime_error("Compute dispatch replaced its reusable descriptor set");
        }
    }
}
