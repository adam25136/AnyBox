#include <anybox/shader/runtime/ComputeBinding.hpp>
#include "Commands.hpp"
#include <cstring>

namespace {
template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); }
    catch (const TException&) { return; }
    throw std::runtime_error("External shader data accepted an invalid argument");
}

void Run(AnyBox::Vulkan::Context& context, unsigned mask) {
    using namespace AnyBox;
    std::string source = "#version 450\n#extension GL_EXT_buffer_reference : require\nlayout(local_size_x=1) in;\n";
    source += "layout(buffer_reference,std430,buffer_reference_align=4) buffer Output { uint values[]; };\nlayout(push_constant) uniform Parameters { Output outputBuffer; } args;\n";
    source += "layout(set=0,binding=62,std430) readonly buffer Data { uint words[]; } data;\n";
    if (mask & 1) source += "layout(set=0,binding=59,std430) readonly buffer Pages { uint words[]; } pages;\n";
    if (mask & 2) source += "layout(set=0,binding=60,std430) readonly buffer Faults { uint words[]; } faults;\n";
    source += "void main() { uint sum=0; for(uint i=1;i<33;++i) sum+=data.words[i]; args.outputBuffer.values[data.words[0]]=sum";
    if (mask & 1) source += "+pages.words[0]";
    if (mask & 2) source += "+faults.words[0]";
    source += "; }\n";
    Shader::ComputeArtifact artifact;
    artifact.sourceHash = Shader::HashShader(std::as_bytes(std::span(source.data(), source.size())));
    artifact.threads = {1, 1, 1};
    artifact.pushConstantSize = 8;
    artifact.memoryOffset = 33;
    for (unsigned index = 0; index < 33; ++index) artifact.userRegisters.push_back(index);
    artifact.descriptors.push_back({62, {}});
    for (unsigned index = 0; index < 2; ++index) if (mask & (1u << index)) artifact.descriptors.push_back({59 + index, {}});
    artifact.code = Vulkan::Tests::CompileCompute(source.c_str());
    Shader::Runtime::ComputePipeline pipeline(context, artifact);
    Shader::Runtime::ComputeMemory memory(context, Shader::Runtime::MemoryTable({}));
    Shader::Runtime::ComputeBinding binding(pipeline, memory);
    if (pipeline.GetShaderDataSize() != 132 || pipeline.GetDescriptorMask() != (mask | 8)) throw std::runtime_error("External shader data layout is incorrect");
    constexpr unsigned count = 600;
    auto output = Vulkan::Buffer::Allocate(context, count * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    auto address = output.GetAddress();
    std::array constants{static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32)};
    std::array<std::uint32_t, 33> arguments{};
    for (unsigned submission = 0; submission < 2; ++submission) {
        Vulkan::Tests::Commands commands(context);
        auto command = commands.Begin();
        memory.Begin(command);
        Reject<std::invalid_argument>([&] { pipeline.GetPipeline().Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1}, binding.GetSets()); });
        for (unsigned index = 0; index < count; ++index) {
            arguments[0] = index;
            for (unsigned word = 1; word < arguments.size(); ++word) arguments[word] = (index + submission * 1000) * word + 0x1234;
            binding.Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1}, std::as_bytes(std::span(arguments)));
        }
        arguments.fill(0xDEADBEEF);
        Reject<std::invalid_argument>([&] { binding.Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1}); });
        Reject<std::invalid_argument>([&] { binding.Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1}, std::as_bytes(std::span(arguments)).first(128)); });
        memory.End(command);
        VkMemoryBarrier visible{};
        visible.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        visible.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        visible.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &visible, 0, nullptr, 0, nullptr);
        commands.Submit(command);
        memory.CheckFault();
        for (unsigned index = 0; index < count; ++index) {
            std::uint32_t actual;
            std::memcpy(&actual, output.Map().data() + index * 4, 4);
            auto expected = (index + submission * 1000) * 528 + 32 * 0x1234 + (mask & 1);
            if (actual != expected) throw std::runtime_error("External shader data dispatch lost its recorded argument snapshot");
        }
    }
    auto reject = [&](auto invalid) { Reject<std::runtime_error>([&] { Shader::Runtime::ComputePipeline selected(context, invalid); }); };
    artifact.memoryOffset = 0;
    artifact.userRegisters.clear();
    reject(artifact);
    artifact.memoryOffset = 33;
    artifact.pushDataStart = 0;
    artifact.pushConstantSize = 128;
    Reject<std::invalid_argument>([&] { Shader::Runtime::ComputePipeline selected(context, artifact); });
}
}

void CheckExternalShaderData(AnyBox::Vulkan::Context& context) { for (unsigned mask = 0; mask < 4; ++mask) Run(context, mask); }
