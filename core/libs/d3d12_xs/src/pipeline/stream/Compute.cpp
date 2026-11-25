#include "Description.hpp"

D3D12_COMPUTE_PIPELINE_STATE_DESC AnyBox::Graphics::PipelineStream::GetComputeDescription() const {
    if (GetKind() != PipelineStreamKind::Compute) throw std::invalid_argument("Xbox pipeline stream is not a compute pipeline");
    for (auto type : {7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u, 21u, 22u}) if (Has(type)) throw std::runtime_error("Compute pipeline stream contains a graphics state subobject");
    D3D12_COMPUTE_PIPELINE_STATE_DESC description{};
    if (Has(0)) description.pRootSignature = Read<ID3D12RootSignature*>(0);
    description.CS = GetShader(6);
    if (Has(18)) description.NodeMask = Read<std::uint32_t>(18);
    if (Has(19)) description.CachedPSO = Read<D3D12_CACHED_PIPELINE_STATE>(19);
    if (Has(20)) description.Flags = Read<D3D12_PIPELINE_STATE_FLAGS>(20);
    return description;
}
