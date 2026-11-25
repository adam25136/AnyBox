#include "Descriptors.hpp"
#include "ComputeCompiler.hpp"
#include "Commands.hpp"
#include "Fixtures.hpp"
#include "Target.hpp"
#include "GpuValues.hpp"
#include <anybox/vulkan/ComputePipeline.hpp>
#include <anybox/shader/runtime/MemoryTable.hpp>
#include <BdaAbi.hpp>
#include <cstring>
#include <algorithm>
#include <vector>

namespace BufferTests {
std::vector<std::uint32_t> ReadGpu(AnyBox::Vulkan::Context& context, const AnyBox::Shader::ComputeCompilation& shader, std::size_t count, const std::array<std::uint32_t, 3>& groups, std::span<const std::byte> inputData, std::span<const std::uint32_t> userData, std::span<const std::byte> outputData) {
    namespace Abi = ShaderRecompiler::BdaAbi;
    using namespace AnyBox::Vulkan;
    if (!context.SupportsByteStorage()) throw std::runtime_error("DXIL BDA test requires Vulkan byte storage support");
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto input = std::make_shared<Buffer>(Buffer::Allocate(context, std::max<std::size_t>(64, inputData.size()), usage, properties));
    auto output = std::make_shared<Buffer>(Buffer::Allocate(context, count * sizeof(std::uint32_t), usage, properties));
    auto faults = Buffer::Allocate(context, Abi::FaultBufferBytes, usage, properties);
    AnyBox::Graphics::Buffers::AddressSpace addresses;
    auto outputPermissions = Abi::Write | (outputData.empty() ? 0 : Abi::Read);
    using AnyBox::Shader::Runtime::MemoryBinding;
    std::array bindings{
        MemoryBinding{addresses.Register(0x100000000, 48, input->GetAddress(), input), Abi::Read},
        MemoryBinding{addresses.Register(0x100000020, input->GetSize() - 32, input->GetAddress() + 32, input), Abi::Read},
        MemoryBinding{addresses.Register(0x100000010, 16, input->GetAddress() + 16, input), Abi::Read},
        MemoryBinding{addresses.Register(0x200000000, output->GetSize(), output->GetAddress(), output), outputPermissions}
    };
    AnyBox::Shader::Runtime::MemoryTable pages(bindings);
    auto table = Buffer::Allocate(context, pages.GetByteSize(), usage, properties);
    for (std::size_t index = 0; index < input->Map().size(); ++index) input->Map()[index] = static_cast<std::byte>((index * 3 + 7) & 255);
    if (!inputData.empty()) {
        if (inputData.size() != input->GetSize()) throw std::runtime_error("DXIL GPU test input data has an invalid size");
        std::memcpy(input->Map().data(), inputData.data(), inputData.size());
    }
    std::memset(output->Map().data(), 255, output->Map().size());
    if (!outputData.empty()) {
        if (outputData.size() != output->GetSize()) throw std::runtime_error("DXIL GPU test output data has an invalid size");
        std::memcpy(output->Map().data(), outputData.data(), outputData.size());
    }
    std::memset(faults.Map().data(), 0, faults.Map().size());
    pages.Write(table.Map());
    Descriptors descriptors(context, table, faults);
    std::array layouts{descriptors.GetLayout()};
    std::array sets{descriptors.GetSet()};
    ComputePipeline pipeline(context, shader.code, shader.bindings.pushConstantSizeBytes, "main", layouts);
    AnyBox::Vulkan::Tests::Commands commands(context);
    auto buffer = commands.Begin();
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    context.GetFunctions().pipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    std::vector<std::byte> constants(shader.bindings.pushConstantSizeBytes);
    const auto& registers = shader.bindings.layout.userDataRegisters;
    if (!registers.empty() && !shader.bindings.layout.UsesPushData()) throw std::runtime_error("GPU argument test requires inline shader data");
    for (std::size_t index = 0; index < registers.size(); ++index) {
        if (registers[index] >= userData.size() || (index + 1) * 4 > constants.size()) throw std::runtime_error("GPU test user data is incomplete");
        std::memcpy(constants.data() + index * 4, &userData[registers[index]], 4);
    }
    pipeline.Dispatch(buffer, constants, groups, sets);
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.GetFunctions().pipelineBarrier(buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    commands.Submit(buffer);
    Abi::Fault fault{};
    std::memcpy(&fault, faults.Map().data(), sizeof(fault));
    if (fault.state != Abi::FaultState::Empty) throw std::runtime_error("DXIL BDA GPU test recorded fault " + std::to_string(static_cast<unsigned>(fault.reason)));
    std::vector<std::uint32_t> values(count);
    std::memcpy(values.data(), output->Map().data(), values.size() * sizeof(std::uint32_t));
    return values;
}

void RunGpu(AnyBox::Vulkan::Context& context, const AnyBox::Shader::ComputeCompilation& shader, std::span<const std::uint32_t> expected, const std::array<std::uint32_t, 3>& groups, std::span<const std::byte> inputData, std::span<const std::uint32_t> userData, std::span<const std::byte> outputData) {
    auto values = ReadGpu(context, shader, expected.size(), groups, inputData, userData, outputData);
    for (std::size_t index = 0; index < expected.size(); ++index) {
        auto value = values[index];
        if (value != expected[index]) {
            std::string first;
            for (std::size_t word = 0; word < 3 && word < expected.size(); ++word) {
                first += " " + std::to_string(values[word]);
            }
            throw std::runtime_error("DXIL GPU result mismatch at " + std::to_string(index) + ": " + std::to_string(value) + " != " + std::to_string(expected[index]) + "; first words:" + first);
        }
    }
}

void CheckGpu(AnyBox::Vulkan::Context& context, const AnyBox::Shader::Dxil::ComputeCompilation& shader, std::uint32_t expected) {
    RunGpu(context, shader, std::span(&expected, 1), {1, 1, 1});
}
}
