#include "GpuQueueFixture.hpp"
#include "../../src/allocator/Allocator.hpp"
#include <anybox/vulkan/Buffer.hpp>
#include <cstring>

void AnyBox::Graphics::Tests::CheckCommandArena(Device* device) {
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateCommandAllocator)>(device, 9)(device, D3D12_COMMAND_LIST_TYPE_DIRECT, AllocatorId, &output) == S_OK && output, "Vulkan command allocator creation failed");
    auto release = [](Allocator* value) { Method<decltype(&Allocator::Release)>(value, 2)(value); };
    std::unique_ptr<Allocator, decltype(release)> allocator(static_cast<Allocator*>(output), release);
    auto reset = Method<decltype(&Allocator::Reset)>(allocator.get(), 8);
    auto& arena = allocator->GetGpuArena();
    auto& context = device->GetGpuContext();
    auto buffer = std::make_shared<Vulkan::Buffer>(Vulkan::Buffer::Allocate(context, 16, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
    std::weak_ptr<Vulkan::Buffer> retained = buffer;
    auto* observed = buffer.get();
    auto record = arena.Begin();
    Reject<std::logic_error>([&] { (void)arena.Begin(); });
    Reject<std::logic_error>([&] { reset(allocator.get()); });
    Reject<std::logic_error>([&] { record.GetExecutable(); });
    Reject<std::invalid_argument>([&] { record.Retain({}); });
    record.Retain(buffer);
    buffer.reset();
    auto commands = record.GetCommands();
    context.GetFunctions().fillBuffer(commands, observed->GetHandle(), 0, 16, 0x78563412);
    VkMemoryBarrier visible{};
    visible.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    visible.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    visible.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.GetFunctions().pipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &visible, 0, nullptr, 0, nullptr);
    record.Close();
    Reject<std::logic_error>([&] { record.Close(); });
    Reject<std::logic_error>([&] { record.GetCommands(); });
    auto moved = std::move(record);
    Reject<std::logic_error>([&] { record.GetExecutable(); });
    {
        GpuQueueFixture fixture(device);
        Vulkan::QueueBridge bridge(context, *fixture.GetQueue().GetHost(), 0);
        Vulkan::CommandArena foreign(context);
        Reject<std::invalid_argument>([&] { (void)foreign.LockSubmission(moved, bridge); });
        {
            QueueGate unblock{fixture.GetFence(), 7};
            fixture.Wait(fixture.GetFence(), 7);
            struct Submitted {};
            try {
                auto submission = arena.LockSubmission(moved, bridge);
                auto submitted = submission.GetCommands();
                bridge.Submit(std::span(&submitted, 1));
                throw Submitted{};
            } catch (const Submitted&) {}
            Reject<std::logic_error>([&] { reset(allocator.get()); });
            Require(!retained.expired(), "Pending Vulkan commands released their retained buffer");
            fixture.Signal(11);
            fixture.Register(11);
            Require(fixture.Await(20) == WAIT_TIMEOUT, "Vulkan allocator test did not retain its queued dependency");
        }
        Require(fixture.Await(5000) == WAIT_OBJECT_0, "Vulkan command arena submission did not complete");
        std::uint32_t value;
        std::memcpy(&value, observed->Map().data(), sizeof(value));
        Require(value == 0x78563412 && !retained.expired(), "Vulkan command arena lost its resource before reset");
        { auto cancelled = arena.LockReset(); }
        Require(moved.GetExecutable() == commands, "Cancelled allocator reset invalidated its command record");
    }
    Require(reset(allocator.get()) == S_OK && retained.expired(), "Completed Vulkan allocator reset did not release its retained resource");
    Reject<std::logic_error>([&] { moved.GetExecutable(); });
    {
        auto reused = arena.Begin();
        Require(reused.GetCommands() == commands, "Vulkan allocator reset did not reuse its existing command buffer");
    }
    Require(reset(allocator.get()) == S_OK, "Abandoned Vulkan command recording prevented allocator reset");
}
