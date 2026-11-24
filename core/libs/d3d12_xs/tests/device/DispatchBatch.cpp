#include "DispatchBatch.hpp"
#include "ComputeFixture.hpp"
#include "DispatchBuffer.hpp"
#include "GpuQueueFixture.hpp"

void AnyBox::Graphics::Tests::CheckDispatchBatch(Device* device, ComputeFixture& fixture, void* pipeline, GpuQueueFixture& queue, DispatchBuffer& buffer) {
    auto release = [](CommandList* value) { Method<decltype(&CommandList::Release)>(value, 2)(value); };
    std::vector<std::unique_ptr<CommandList, decltype(release)>> retained;
    std::vector<void*> lists{fixture.GetList()};
    for (unsigned index = 1; index <= 18; ++index) {
        void* output = nullptr;
        Require(Method<decltype(&Device::CreateCommandList)>(device, 12)(device, 0, D3D12_COMMAND_LIST_TYPE_COMPUTE, fixture.GetAllocator(), pipeline, XboxGraphicsListId, &output) == S_OK && output, "Prepared dispatch batch list creation failed");
        auto* list = static_cast<CommandList*>(output);
        retained.emplace_back(list, release);
        lists.push_back(list);
        Method<decltype(&CommandList::SetComputeRootSignature)>(list, 33)(list, fixture.GetRoot());
        Method<decltype(&CommandList::SetComputeRootUnorderedAccessView)>(list, 45)(list, 0, buffer.GetAddress());
        std::array values{0xA000u + index, index * 4};
        Method<decltype(&CommandList::SetComputeRoot32BitConstants)>(list, 39)(list, 3, 2, values.data(), 0);
        Method<decltype(&CommandList::Dispatch)>(list, 14)(list, 1, 1, 1);
        Require(Method<decltype(&CommandList::Close)>(list, 9)(list) == S_OK, "Prepared dispatch batch list close failed");
    }
    Method<decltype(&Queue::ExecuteCommandLists)>(&queue.GetQueue(), 10)(&queue.GetQueue(), static_cast<std::uint32_t>(lists.size()), lists.data());
    queue.Signal(13);
    queue.Register(13);
    Require(queue.Await(5000) == WAIT_OBJECT_0, "Prepared dispatch batch did not finish");
    for (unsigned index = 1; index <= 18; ++index) Require(buffer.Read(index) == 0xA000 + index, "Prepared dispatch batch lost a command from a shared allocator");
}
