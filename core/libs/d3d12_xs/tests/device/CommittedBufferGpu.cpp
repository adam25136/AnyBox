#include "CommittedBuffers.hpp"
#include "../../../../graphics/vulkan/tests/Commands.hpp"
#include <algorithm>

void AnyBox::Graphics::Tests::CheckCommittedBufferGpu(Device* device) {
    constexpr std::size_t size = 65536;
    auto upload = MakeCommittedBuffer(device, D3D12_HEAP_TYPE_UPLOAD, size);
    auto target = MakeCommittedBuffer(device, D3D12_HEAP_TYPE_DEFAULT, size);
    auto readback = MakeCommittedBuffer(device, D3D12_HEAP_TYPE_READBACK, size);
    void* source = nullptr;
    void* destination = nullptr;
    Require(Method<decltype(&Resource::Map)>(upload.get(), 8)(upload.get(), 0, nullptr, &source) == S_OK, "Committed upload mapping failed");
    Require(Method<decltype(&Resource::Map)>(readback.get(), 8)(readback.get(), 0, nullptr, &destination) == S_OK, "Committed readback mapping failed");
    std::vector<std::byte> expected(size);
    for (std::size_t index = 0; index < size; ++index) expected[index] = static_cast<std::byte>((index * 37 + 19) & 255);
    std::memcpy(source, expected.data(), size);
    auto& context = device->GetGpuContext();
    const auto& functions = context.GetFunctions();
    auto retained = device->GetBufferAddresses().Resolve(reinterpret_cast<std::uintptr_t>(destination), size);
    auto destinationHandle = readback->GetGpuBuffer().GetHandle();
    readback.reset();

    Vulkan::Tests::Commands commands(context);
    auto buffer = commands.Begin();
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    VkBufferCopy copy{0, 0, size};
    functions.copyBuffer(buffer, upload->GetGpuBuffer().GetHandle(), target->GetGpuBuffer().GetHandle(), 1, &copy);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    functions.copyBuffer(buffer, target->GetGpuBuffer().GetHandle(), destinationHandle, 1, &copy);
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    commands.Submit(buffer);
    Require(std::ranges::equal(expected, std::span(static_cast<std::byte*>(destination), size)), "Committed GPU buffer transfer changed the data");
    MEMORY_BASIC_INFORMATION information{};
    Require(VirtualQuery(destination, &information, sizeof(information)) != 0 && information.State == MEM_COMMIT, "Retained GPU mapping lost its allocation");
    retained.reset();
    Require(VirtualQuery(destination, &information, sizeof(information)) != 0 && information.State == MEM_FREE, "Committed allocation outlived its final GPU mapping");
}
