#include "ComputeFixture.hpp"
#include "../../src/pipeline/State.hpp"

namespace {
void CheckCache(AnyBox::Graphics::CommandList* list, AnyBox::Graphics::PipelineState* pipeline) {
    std::array<std::uint32_t, 8> cache{};
    std::memcpy(cache.data(), reinterpret_cast<const std::byte*>(list) + 0x544, sizeof(cache));
    AnyBox::Graphics::Tests::Require(cache == pipeline->GetInlineCommands(), "Initial pipeline inline cache differs from its state");
}
}

void AnyBox::Graphics::Tests::CheckInitialPipelines(Device* device, void* object) {
    auto* pipeline = static_cast<PipelineState*>(object);
    auto addRef = Method<decltype(&PipelineState::AddRef)>(pipeline, 1);
    auto release = Method<decltype(&PipelineState::Release)>(pipeline, 2);
    ComputeFixture fixture(device);
    auto* list = fixture.GetList();
    auto close = Method<decltype(&CommandList::Close)>(list, 9);
    auto reset = Method<decltype(&CommandList::Reset)>(list, 10);
    Require(close(list) == S_OK, "Initial pipeline fixture close failed");
    Reject<std::invalid_argument>([&] { reset(list, fixture.GetAllocator(), fixture.GetRoot()); });
    Require(list->IsClosed() && !list->GetPipelineState(), "Rejected initial pipeline changed the list state");
    for (unsigned iteration = 0; iteration < 2; ++iteration) {
        Require(reset(list, fixture.GetAllocator(), pipeline) == S_OK, "Reset with an initial pipeline failed");
        Require(list->GetPipelineState() == pipeline && !list->GetComputeState(), "Reset lost its initial pipeline or implicitly bound its root signature");
        CheckCache(list, pipeline);
        Require(addRef(pipeline) == 3, "Reset initial pipeline reference count is inconsistent");
        Require(release(pipeline) == 2, "Reset initial pipeline reference was not released");
        Require(close(list) == S_OK, "Initial pipeline list close failed");
    }
    Require(reset(list, fixture.GetAllocator(), nullptr) == S_OK && close(list) == S_OK, "Reset without a pipeline failed");
    Require(addRef(pipeline) == 2, "Reset without a pipeline retained the previous state");
    release(pipeline);

    for (auto type : {D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_COMPUTE}) {
        void* output = nullptr;
        Require(Method<decltype(&Device::CreateCommandAllocator)>(device, 9)(device, type, AllocatorId, &output) == S_OK, "Initial pipeline allocator creation failed");
        auto* allocator = static_cast<Allocator*>(output);
        for (bool xbox : {false, true}) {
            HRESULT result;
            if (xbox) {
                XboxListDescription description{type, 2, 1, 0};
                result = Method<decltype(&Device::CreateCommandListX)>(device, 73)(device, &description, allocator, pipeline, XboxGraphicsListId, &output);
            } else result = Method<decltype(&Device::CreateCommandList)>(device, 12)(device, 0, type, allocator, pipeline, XboxGraphicsListId, &output);
            Require(result == S_OK && output, "Command list creation with an initial pipeline failed");
            auto* created = static_cast<CommandList*>(output);
            Require(created->GetPipelineState() == pipeline && !created->GetComputeState(), "Created list lost its initial pipeline or bound an implicit root");
            CheckCache(created, pipeline);
            Require(addRef(pipeline) == 3, "Created list did not retain its initial pipeline");
            release(pipeline);
            Require(Method<decltype(&CommandList::Close)>(created, 9)(created) == S_OK, "Created initial pipeline list close failed");
            Require(Method<decltype(&CommandList::Release)>(created, 2)(created) == 0, "Created initial pipeline list was not released");
        }
        Require(Method<decltype(&Allocator::Release)>(allocator, 2)(allocator) == 0, "Initial pipeline allocator retained a reference");
    }
    void* output = fixture.GetRoot();
    Reject<std::invalid_argument>([&] { Method<decltype(&Device::CreateCommandList)>(device, 12)(device, 0, D3D12_COMMAND_LIST_TYPE_COMPUTE, fixture.GetAllocator(), fixture.GetRoot(), XboxGraphicsListId, &output); });
    Require(!output, "Rejected initial pipeline retained a command list output");
    for (auto type : {D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_LIST_TYPE_BUNDLE}) {
        Method<decltype(&Device::CreateCommandAllocator)>(device, 9)(device, type, AllocatorId, &output);
        auto* allocator = static_cast<Allocator*>(output);
        Reject<std::runtime_error>([&] { Method<decltype(&Device::CreateCommandList)>(device, 12)(device, 0, type, allocator, pipeline, GraphicsListId, &output); });
        Require(!output, "Unsupported initial pipeline list retained an output");
        Method<decltype(&Allocator::Release)>(allocator, 2)(allocator);
    }
}
