#include "ComputeFixture.hpp"
#include "../../src/pipeline/State.hpp"
#include <algorithm>

void AnyBox::Graphics::Tests::CheckPipelineBindings(Device* device, void* object) {
    auto* pipeline = static_cast<PipelineState*>(object);
    auto addRef = Method<decltype(&PipelineState::AddRef)>(pipeline, 1);
    auto release = Method<decltype(&PipelineState::Release)>(pipeline, 2);
    ComputeFixture fixture(device);
    auto* list = fixture.GetList();
    auto bind = Method<decltype(&CommandList::SetPipelineState)>(list, 25);
    auto close = Method<decltype(&CommandList::Close)>(list, 9);
    auto reset = Method<decltype(&CommandList::Reset)>(list, 10);
    Require(!list->GetPipelineState(), "New command list retained a pipeline");
    Reject<std::invalid_argument>([&] { bind(list, nullptr); });
    Reject<std::invalid_argument>([&] { bind(list, fixture.GetRoot()); });
    bind(list, pipeline);
    Require(list->GetPipelineState() == pipeline, "Slot 25 lost its pipeline binding");
    Require(addRef(pipeline) == 3, "Pipeline binding did not retain a reference");
    Require(release(pipeline) == 2, "Pipeline binding reference accounting differs");
    bind(list, pipeline);
    Require(addRef(pipeline) == 3, "Rebinding the same pipeline added a reference");
    release(pipeline);
    std::array<std::uint32_t, 8> cache{};
    std::memcpy(cache.data(), reinterpret_cast<const std::byte*>(list) + 0x544, sizeof(cache));
    Require(cache == pipeline->GetInlineCommands(), "Pipeline inline state cache is inconsistent");
    Require(close(list) == S_OK, "Pipeline list close failed");
    Reject<std::logic_error>([&] { bind(list, pipeline); });
    Require(reset(list, fixture.GetAllocator(), nullptr) == S_OK, "Pipeline list reset failed");
    Require(!list->GetPipelineState(), "Reset retained the previous pipeline");
    std::memcpy(cache.data(), reinterpret_cast<const std::byte*>(list) + 0x544, sizeof(cache));
    Require(std::ranges::all_of(cache, [](auto word) { return word == 0; }), "Reset retained the pipeline inline cache");

    auto* begin = ComputeFixture::GetWrite(list);
    std::copy_n(pipeline->GetInlineCommands().begin(), 7, begin);
    begin[7] = 0x12345678;
    ComputeFixture::SetWrite(list, begin + 8);
    Reject<std::runtime_error>([&] { close(list); });
    Require(!list->GetPipelineState() && ComputeFixture::GetWrite(list) == begin + 8, "Rejected inline stream changed pipeline state");
    Require(addRef(pipeline) == 2, "Rejected inline stream retained pipeline references");
    release(pipeline);
    ComputeFixture::SetWrite(list, begin + 7);
    Require(close(list) == S_OK && list->GetPipelineState() == pipeline, "Rootless inline pipeline binding failed");
    Require(ComputeFixture::GetWrite(list) == begin, "Inline pipeline binding was not consumed");
}
