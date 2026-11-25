#include "ComputeFixture.hpp"
#include "PreparedCache.hpp"
#include "DispatchBuffer.hpp"
#include "DispatchArtifact.hpp"
#include "DispatchBatch.hpp"
#include "GpuQueueFixture.hpp"
#include "../../src/pipeline/State.hpp"
#include <algorithm>
#include <string_view>

namespace AnyBox::Graphics::Tests {
static void CheckPreparedDispatchPath(Device* device, bool external) {
    D3D12_FEATURE_DATA_D3D12_OPTIONS4 capabilities{};
    Require(device->GetHost()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &capabilities, sizeof(capabilities)) == S_OK, "Prepared dispatch capability query failed");
    if (capabilities.Native16BitShaderOpsSupported) return;
    DispatchBuffer buffer(device);
    ComputeFixture fixture(device);
    PreparedCache cache;
    auto data = MakeDispatchArtifact(external);
    cache.Save(data.artifact);
    D3D12_COMPUTE_PIPELINE_STATE_DESC description{};
    description.pRootSignature = reinterpret_cast<ID3D12RootSignature*>(fixture.GetRoot());
    description.CS = {data.source.data(), data.source.size()};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateComputePipelineStateX)>(device, 62)(device, &description, 0, nullptr, PipelineStateId, &output) == S_OK && output, "Prepared dispatch pipeline creation failed");
    auto* pipeline = static_cast<PipelineState*>(output);
    auto release = [](PipelineState* value) { Method<decltype(&PipelineState::Release)>(value, 2)(value); };
    std::unique_ptr<PipelineState, decltype(release)> owned(pipeline, release);
    GpuQueueFixture queue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    auto* list = fixture.GetList();
    auto bindRoot = Method<decltype(&CommandList::SetComputeRootSignature)>(list, 33);
    auto bindBuffer = Method<decltype(&CommandList::SetComputeRootUnorderedAccessView)>(list, 45);
    auto constants = Method<decltype(&CommandList::SetComputeRoot32BitConstants)>(list, 39);
    auto close = Method<decltype(&CommandList::Close)>(list, 9);
    auto dispatch = Method<decltype(&CommandList::Dispatch)>(list, 14);
    auto reset = Method<decltype(&Allocator::Reset)>(fixture.GetAllocator(), 8);
    bindRoot(list, fixture.GetRoot());
    Method<decltype(&CommandList::SetPipelineState)>(list, 25)(list, pipeline);
    bindBuffer(list, 0, buffer.GetAddress());
    std::uint32_t expected = 1;
    for (auto header : {0x4002C400u, 0x4002E800u, 0x4002E804u}) {
        auto* words = ComputeFixture::GetWrite(list);
        std::array packet{ComputeFixture::ReadHeader(list, 3) | (1u << 16), expected++, 0u, header, 1u, 1u, 1u};
        std::copy(packet.begin(), packet.end(), words);
        ComputeFixture::SetWrite(list, words + packet.size());
        Method<decltype(&CommandList::SyncTopOfPipeX)>(list, 131)(list, 0xC380, 0x20000, 0, (1ULL << 48) - 256);
    }
    std::array values{0x12345678u, 0u};
    constants(list, 3, static_cast<std::uint32_t>(values.size()), values.data(), 0);
    dispatch(list, 1, 1, 1);
    Require(close(list) == S_OK, "Prepared dispatch command list close failed");
    void* lists[]{list};
    auto execute = Method<decltype(&Queue::ExecuteCommandLists)>(&queue.GetQueue(), 10);
    execute(&queue.GetQueue(), 1, lists);
    queue.Signal(11);
    queue.Register(11);
    Require(queue.Await(5000) == WAIT_OBJECT_0 && buffer.Read(0) == values[0], "Xbox inline and vtable dispatch did not reach the Vulkan buffer");
    execute(&queue.GetQueue(), 1, lists);
    queue.Signal(12);
    queue.Register(12);
    Require(queue.Await(5000) == WAIT_OBJECT_0, "Prepared command list replay did not complete");
    CheckDispatchBatch(device, fixture, pipeline, queue, buffer);
    Require(reset(fixture.GetAllocator()) == S_OK, "Completed prepared command allocator reset failed");
    Reject<std::logic_error>([&] { execute(&queue.GetQueue(), 1, lists); });
    Require(Method<decltype(&CommandList::Reset)>(list, 10)(list, fixture.GetAllocator(), pipeline) == S_OK, "Prepared dispatch recording reset failed");
    bindRoot(list, fixture.GetRoot());
    bindBuffer(list, 0, buffer.GetAddress());
    values = {0x55u, static_cast<std::uint32_t>(2 * PhysicalPageSize)};
    constants(list, 3, static_cast<std::uint32_t>(values.size()), values.data(), 0);
    dispatch(list, 1, 1, 1);
    Require(close(list) == S_OK, "Faulting prepared dispatch close failed");
    execute(&queue.GetQueue(), 1, lists);
    execute(&queue.GetQueue(), 1, lists);
    queue.Signal(15);
    queue.Register(15);
    Require(queue.Await(5000) == WAIT_OBJECT_0, "Faulting prepared GPU work did not complete");
    bool fault = false;
    int destructors = 0;
    try { AnyBox::Tests::Invoke([&] { reset(fixture.GetAllocator()); }, destructors); }
    catch (const std::runtime_error& error) { fault = std::string_view(error.what()).find("GPU shader fault") != std::string_view::npos; }
    Require(fault && destructors == 1, "Prepared dispatch GPU fault did not unwind the allocator caller");
    Require(Method<decltype(&Device::GetDeviceRemovedReason)>(device, 37)(device) == S_OK, "Contained shader fault removed the native device");
}

void CheckPreparedDispatch(Device* device) { for (bool external : {false, true}) CheckPreparedDispatchPath(device, external); }
}
