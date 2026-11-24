#include "GpuQueueFixture.hpp"
#include <anybox/vulkan/Buffer.hpp>
#include <cstring>

namespace {
void Visible(AnyBox::Vulkan::Context& context, VkCommandBuffer command) {
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    AnyBox::Vulkan::Check(context.GetFunctions().endCommandBuffer(command), "End queue bridge transfer test commands");
}

void CheckValue(AnyBox::Vulkan::Buffer& buffer, std::uint32_t expected) {
    for (unsigned index = 0; index < 4; ++index) {
        std::uint32_t value;
        std::memcpy(&value, buffer.Map().data() + index * 4, sizeof(value));
        AnyBox::Graphics::Tests::Require(value == expected, "GPU queue bridge transfer produced an incorrect value");
    }
}
}

void AnyBox::Graphics::Tests::CheckGpuQueue(Device* device) {
    auto& context = device->GetGpuContext();
    constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto output = Vulkan::Buffer::Allocate(context, 16, usage, properties);
    auto copied = Vulkan::Buffer::Allocate(context, 16, usage, properties);
    QueueCommands pool(context);
    auto fill = pool.Begin();
    context.GetFunctions().fillBuffer(fill, output.GetHandle(), 0, 16, 0x12345678);
    Visible(context, fill);
    auto copy = pool.Begin();
    VkBufferCopy region{0, 0, 16};
    context.GetFunctions().copyBuffer(copy, output.GetHandle(), copied.GetHandle(), 1, &region);
    Visible(context, copy);
    {
        GpuQueueFixture fixture(device);
        std::memset(output.Map().data(), 0, 16);
        DWORD premature;
        {
            QueueGate unblock{fixture.GetFence(), 7};
            fixture.Wait(fixture.GetFence(), 7);
            fixture.GetQueue().SubmitGpu(std::span(&fill, 1));
            fixture.Signal(11);
            fixture.Register(11);
            premature = fixture.Await(20);
            CheckValue(output, 0);
        }
        Require(fixture.Await(5000) == WAIT_OBJECT_0 && premature == WAIT_TIMEOUT, "Vulkan work bypassed a preceding Xbox queue wait");
        CheckValue(output, 0x12345678);
        for (unsigned repeat = 0; repeat < 2; ++repeat) {
            fixture.GetQueue().SubmitGpu(std::span(&copy, 1));
            fixture.Signal(13 + repeat);
            fixture.Register(13 + repeat);
            Require(fixture.Await(5000) == WAIT_OBJECT_0, "GPU queue bridge did not deliver its subsequent client fence signal");
            CheckValue(copied, 0x12345678);
        }
        VkCommandBuffer invalid = VK_NULL_HANDLE;
        Reject<std::invalid_argument>([&] { fixture.GetQueue().SubmitGpu(std::span(&invalid, 1)); });
    }
    {
        std::memset(output.Map().data(), 0, 16);
        std::memset(copied.Map().data(), 0, 16);
        GpuQueueFixture producer(device);
        GpuQueueFixture consumer(device);
        QueueGate unblock{producer.GetFence(), 31};
        consumer.Wait(producer.GetFence(), 31);
        consumer.GetQueue().SubmitGpu(std::span(&copy, 1));
        consumer.Signal(37);
        consumer.Register(37);
        producer.GetQueue().SubmitGpu(std::span(&fill, 1));
        producer.Signal(31);
        Require(consumer.Await(5000) == WAIT_OBJECT_0, "Independent Vulkan queues did not resolve their Xbox fence dependency");
        CheckValue(copied, 0x12345678);
    }
}
