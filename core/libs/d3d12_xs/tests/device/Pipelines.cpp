#include "Tests.hpp"
#include "../../src/pipeline/State.hpp"
#include <d3d12_xs.hpp>
#include <memory>

namespace {
struct BlobRelease {
    void operator()(ID3DBlob* value) const noexcept { value->Release(); }
};
}

void AnyBox::Graphics::Tests::CheckPipelines(Device* device) {
    auto code = CompileComputeShader();
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC layout{};
    layout.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
    layout.Desc_1_0 = {1, &parameter, 0, nullptr, static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(0x03000000)};
    ID3DBlob* serialized = nullptr;
    D3D12SerializeVersionedRootSignature(&layout, &serialized, nullptr);
    std::unique_ptr<ID3DBlob, BlobRelease> blob(serialized);
    void* output = nullptr;
    Method<decltype(&Device::CreateRootSignature)>(device, 16)(device, 0, blob->GetBufferPointer(), blob->GetBufferSize(), RootSignatureId, &output);
    auto* root = static_cast<RootSignature*>(output);
    auto releaseRoot = Method<decltype(&RootSignature::Release)>(root, 2);
    D3D12_COMPUTE_PIPELINE_STATE_DESC description{};
    description.pRootSignature = reinterpret_cast<ID3D12RootSignature*>(root);
    description.CS = {code.data(), code.size()};
    auto create = Method<decltype(&Device::CreateComputePipelineStateX)>(device, 62);
    Require(create(device, &description, 0, nullptr, PipelineStateId, &output) == 0 && output, "Compute pipeline creation through the Xbox ABI failed");
    auto* pipeline = static_cast<PipelineState*>(output);
    CheckPipelineStreams(device, root, code);
    CheckInitialPipelines(device, pipeline);
    CheckPipelineBindings(device, pipeline);
    auto release = Method<decltype(&PipelineState::Release)>(pipeline, 2);
    auto query = Method<decltype(&PipelineState::QueryInterface)>(pipeline, 0);
    Require(pipeline->GetRootSignature() == root && root->GetFlags() == 0x03000000, "Compute pipeline lost its Xbox root signature metadata");
    Require(releaseRoot(root) == 1, "Compute pipeline did not retain its root signature");
    for (const auto& id : {PipelineStateId, UnknownId, ObjectId, DeviceChildId, PageableId}) {
        void* identity = nullptr;
        Require(query(pipeline, id, &identity) == 0 && identity == pipeline, "Compute pipeline interface identity changed");
        Require(release(static_cast<PipelineState*>(identity)) == 1, "Compute pipeline query did not retain its reference");
    }
    Require(Method<decltype(&PipelineState::SetName)>(pipeline, 6)(pipeline, L"AnyBox synthetic compute pipeline") == 0, "Compute pipeline naming failed");
    void* parent = nullptr;
    Require(Method<decltype(&PipelineState::GetDevice)>(pipeline, 7)(pipeline, BaseDeviceId, &parent) == 0 && parent == device, "Compute pipeline device identity changed");
    Method<decltype(&Device::Release)>(device, 2)(static_cast<Device*>(parent));
    auto cache = Method<decltype(&PipelineState::GetCachedBlob)>(pipeline, 8);
    ID3DBlob* cached = serialized;
    Reject<std::runtime_error>([&] { cache(pipeline, &cached); });
    Require(!cached, "Unsupported Xbox cache serialization retained a stale blob");
    Reject<std::invalid_argument>([&] { cache(pipeline, nullptr); });
    Reject<std::invalid_argument>([&] { create(device, nullptr, 0, nullptr, PipelineStateId, &output); });
    Require(!output, "Rejected compute pipeline description retained an object");
    Reject<std::invalid_argument>([&] { create(device, &description, 0, nullptr, PipelineStateId, nullptr); });
    Reject<std::invalid_argument>([&] { create(device, &description, 1, nullptr, PipelineStateId, &output); });
    const std::uint32_t extension = 0;
    Reject<std::runtime_error>([&] { create(device, &description, 1, &extension, PipelineStateId, &output); });
    auto invalid = description;
    invalid.CS = {};
    Reject<std::invalid_argument>([&] { create(device, &invalid, 0, nullptr, PipelineStateId, &output); });
    invalid = description;
    invalid.Flags = static_cast<D3D12_PIPELINE_STATE_FLAGS>(0x100);
    Reject<std::invalid_argument>([&] { create(device, &invalid, 0, nullptr, PipelineStateId, &output); });
    invalid = description;
    invalid.NodeMask = 2;
    Reject<std::invalid_argument>([&] { create(device, &invalid, 0, nullptr, PipelineStateId, &output); });
    invalid = description;
    invalid.CachedPSO = {code.data(), code.size()};
    Reject<std::runtime_error>([&] { create(device, &invalid, 0, nullptr, PipelineStateId, &output); });
    Require(release(pipeline) == 0, "Compute pipeline final reference was not released");
}
