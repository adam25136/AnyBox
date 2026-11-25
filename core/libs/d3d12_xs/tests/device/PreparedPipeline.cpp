#include "ComputeFixture.hpp"
#include "PreparedCache.hpp"
#include "../../src/pipeline/State.hpp"
#include <anybox/graphics/container/View.hpp>
#include "../../../../graphics/vulkan/tests/Tests.hpp"

namespace {
std::vector<std::byte> EmptyShader(unsigned wave = 0) {
    std::array<std::uint32_t, 10> words{0x50065, 10, 0x4C495844, 0x105, 28, 4, 0x54414458, 0, 36, 0x43424F4E};
    std::array<std::uint32_t, 18> specified{0x50065, 18, 0x4C495844, 0x105, 60, 4, 0x54414458, 1, 36, 10, 48, 17, 0x706F6278, 1, 627, 1, wave == 64 ? 0x30u : 0x31u, 0x43424F4E};
    auto program = wave == 0 ? std::as_bytes(std::span(words)).first(words.size() * 4) : std::as_bytes(std::span(specified)).first(specified.size() * 4);
    std::array<std::byte, 8> features{};
    return AnyBox::Graphics::Container::BuildDxbc(std::array<AnyBox::Graphics::Container::Part, 2>{{{0x4C495844, program}, {0x30494653, features}}});
}

void CheckWaveMismatch(AnyBox::Graphics::Device* device) {
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    ComputeFixture fixture(device);
    auto code = EmptyShader(64);
    AnyBox::Shader::ComputeArtifact artifact;
    artifact.sourceHash = AnyBox::Shader::HashShader(code);
    artifact.threads = {1, 1, 1};
    artifact.code = AnyBox::Vulkan::Tests::CompileCompute("#version 450\nlayout(local_size_x=1) in; void main() {}\n");
    PreparedCache cache;
    cache.Save(artifact);
    D3D12_COMPUTE_PIPELINE_STATE_DESC description{};
    description.pRootSignature = reinterpret_cast<ID3D12RootSignature*>(fixture.GetRoot());
    description.CS = {code.data(), code.size()};
    auto create = Method<decltype(&Device::CreateComputePipelineStateX)>(device, 62);
    void* output = nullptr;
    bool rejected = false;
    try { create(device, &description, 0, nullptr, PipelineStateId, &output); }
    catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) != "Xbox shader wave size requires explicit subgroup control on this adapter") throw;
        rejected = true;
    }
    Require(rejected && !output, "Prepared shader accepted a source with a different wave size");
}
}

void AnyBox::Graphics::Tests::CheckPreparedPipeline(Device* device) {
    CheckWaveMismatch(device);
    ComputeFixture fixture(device);
    auto code = EmptyShader();
    Shader::ComputeArtifact artifact;
    artifact.sourceHash = Shader::HashShader(code);
    artifact.threads = {1, 1, 1};
    artifact.code = Vulkan::Tests::CompileCompute("#version 450\nlayout(local_size_x=1) in; void main() {}\n");
    PreparedCache cache;
    D3D12_COMPUTE_PIPELINE_STATE_DESC description{};
    description.pRootSignature = reinterpret_cast<ID3D12RootSignature*>(fixture.GetRoot());
    description.CS = {code.data(), code.size()};
    auto create = Method<decltype(&Device::CreateComputePipelineStateX)>(device, 62);
    void* output = nullptr;
    Reject<std::runtime_error>([&] { create(device, &description, 0, nullptr, PipelineStateId, &output); });
    Require(!output, "Missing prepared shader retained a pipeline output");
    cache.Save(artifact);
    Require(create(device, &description, 0, nullptr, PipelineStateId, &output) == S_OK && output, "Prepared shader did not create a pipeline through the Xbox ABI");
    auto* pipeline = static_cast<PipelineState*>(output);
    CheckInitialPipelines(device, pipeline);
    CheckPipelineBindings(device, pipeline);
    Require(pipeline->GetOffline() && pipeline->GetRootSignature() == fixture.GetRoot(), "Prepared pipeline lost its backend or root signature");
    Method<decltype(&PipelineState::SetName)>(pipeline, 6)(pipeline, L"AnyBox prepared compute fixture");
    std::array<std::uint32_t, 8> record;
    std::memcpy(record.data(), reinterpret_cast<const std::byte*>(pipeline) + 16, sizeof(record));
    auto address = record[1] | (static_cast<std::uint64_t>(record[2]) << 32);
    Require(record[0] == 0x8AB00007 && address == reinterpret_cast<std::uintptr_t>(pipeline), "Prepared pipeline inline ABI record is malformed");
    Require(Method<decltype(&PipelineState::Release)>(pipeline, 2)(pipeline) == 0, "Prepared pipeline retained its final reference");
    artifact.runtimeVersion = 13;
    Reject<std::invalid_argument>([&] { cache.Save(artifact); });
}
