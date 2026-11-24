#include "DispatchArtifact.hpp"
#include <anybox/graphics/container/View.hpp>
#include "../../../../graphics/vulkan/tests/Tests.hpp"

namespace {
constexpr auto Source = R"(#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
layout(local_size_x=1) in;
struct Range { uint64_t begin; uint64_t end; uint64_t deviceAddress; uint permissions; uint reserved; };
layout(set=0,binding=59,std430) readonly buffer Pages { uvec4 header; Range ranges[]; } pages;
layout(set=0,binding=60,std430) buffer Faults { uint words[]; } faults;
layout(buffer_reference,std430,buffer_reference_align=4) buffer Output { uint value; };
layout(push_constant) uniform Parameters { uint64_t address; uint64_t size; uint value; uint offset; } args;
void main() {
    uint64_t address = args.address + uint64_t(args.offset);
    uint64_t resolved = uint64_t(0);
    for (uint index=0; index<pages.header.y; ++index) {
        Range range = pages.ranges[index];
        if (address>=range.begin && address+uint64_t(4)<=range.end && (range.permissions&2u)!=0u) resolved=range.deviceAddress+address-range.begin;
    }
    if (resolved!=uint64_t(0)) Output(resolved).value=args.value;
    else if (atomicCompSwap(faults.words[0],0u,1u)==0u) {
        faults.words[1]=1u;
        faults.words[2]=uint(address);
        faults.words[3]=uint(address>>32);
        faults.words[4]=4u;
        memoryBarrierBuffer();
        atomicExchange(faults.words[0],2u);
    }
}
)";
}

AnyBox::Graphics::Tests::DispatchArtifact AnyBox::Graphics::Tests::MakeDispatchArtifact() {
    auto original = CompileComputeShader();
    Container::DxbcView view(original);
    std::vector<Container::Part> parts;
    for (unsigned index = 0; index < view.GetPartCount(); ++index) {
        auto part = view.GetPart(index);
        if (part.tag != 0x30494653) parts.push_back(part);
    }
    std::array features{std::byte{0}, std::byte{0}, std::byte{4}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
    parts.push_back({0x30494653, features});
    DispatchArtifact result;
    result.source = Container::BuildDxbc(parts);
    auto& artifact = result.artifact;
    artifact.sourceHash = Shader::HashShader(result.source);
    artifact.threads = {1, 1, 1};
    artifact.pushConstantSize = 24;
    artifact.pushDataStart = 0;
    artifact.memoryOffset = 6;
    artifact.userRegisters = {0, 1, 2, 3, 4, 5};
    artifact.descriptors = {{59, {}}, {60, {}}};
    artifact.buffers = {{Shader::ResourceClass::UnorderedAccess, 0, 0, 0, 1, Shader::BufferKind::Raw}, {Shader::ResourceClass::ConstantBuffer, 0, 0, 1, 1, Shader::BufferKind::Constant, 0, 8}};
    artifact.code = Vulkan::Tests::CompileCompute(Source);
    return result;
}
