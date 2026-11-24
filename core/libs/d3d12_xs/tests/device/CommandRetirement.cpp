#include "GpuQueueFixture.hpp"
#include <anybox/vulkan/CommandArena.hpp>

void AnyBox::Graphics::Tests::CheckCommandRetirement(Device* device) {
    auto& context = device->GetGpuContext();
    Vulkan::CommandArena arena(context);
    auto record = arena.Begin();
    record.Close();
    GpuQueueFixture fixture(device);
    Vulkan::Submission completion;
    for (unsigned iteration = 0; iteration < 35; ++iteration) {
        Vulkan::QueueBridge bridge(context, *fixture.GetQueue().GetHost(), 0);
        {
            auto submission = arena.LockSubmission(record, bridge);
            auto commands = submission.GetCommands();
            bridge.Submit(std::span(&commands, 1));
        }
        completion = bridge.GetCompletion();
    }
    Require(completion.GetSource() && completion.GetValue() != 0 && completion.IsComplete(), "Vulkan completion marker did not survive queue retirement");
    arena.Reset();
    Reject<std::logic_error>([&] { record.GetExecutable(); });
    auto moved = arena.Begin();
    auto active = std::move(moved);
    Reject<std::logic_error>([&] { moved.Close(); });
    Reject<std::logic_error>([&] { arena.Reset(); });
    active.Close();
    arena.Reset();
}
