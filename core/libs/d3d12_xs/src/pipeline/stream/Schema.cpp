#include "Description.hpp"

namespace {
template<class TValue>
constexpr AnyBox::Graphics::PipelineSubobjectSchema Schema() {
    struct Object { std::uint32_t type; TValue value; };
    return {offsetof(Object, value), sizeof(TValue), (sizeof(Object) + 7) & ~std::size_t{7}};
}
constexpr std::array Schemas{
    Schema<ID3D12RootSignature*>(), Schema<D3D12_SHADER_BYTECODE>(), Schema<D3D12_SHADER_BYTECODE>(),
    Schema<D3D12_SHADER_BYTECODE>(), Schema<D3D12_SHADER_BYTECODE>(), Schema<D3D12_SHADER_BYTECODE>(), Schema<D3D12_SHADER_BYTECODE>(),
    Schema<D3D12_STREAM_OUTPUT_DESC>(), Schema<D3D12_BLEND_DESC>(), Schema<std::uint32_t>(), Schema<D3D12_RASTERIZER_DESC>(),
    Schema<D3D12_DEPTH_STENCIL_DESC>(), Schema<D3D12_INPUT_LAYOUT_DESC>(), Schema<D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>(),
    Schema<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(), Schema<D3D12_RT_FORMAT_ARRAY>(), Schema<DXGI_FORMAT>(), Schema<DXGI_SAMPLE_DESC>(),
    Schema<std::uint32_t>(), Schema<D3D12_CACHED_PIPELINE_STATE>(), Schema<D3D12_PIPELINE_STATE_FLAGS>(),
    Schema<D3D12_DEPTH_STENCIL_DESC1>(), Schema<D3D12_VIEW_INSTANCING_DESC>(), Schema<D3D12_SHADER_BYTECODE>(), Schema<D3D12_SHADER_BYTECODE>()
};
static_assert(Schemas[8].size == 336 && Schemas[8].offset == 4);
static_assert(Schemas[21].size == 64 && Schemas[21].offset == 4);
static_assert(Schemas[1].size == 24 && Schemas[1].offset == 8);
}

AnyBox::Graphics::PipelineSubobjectSchema AnyBox::Graphics::GetPipelineSubobjectSchema(std::uint32_t type) {
    if (type >= Schemas.size()) throw std::runtime_error("Xbox pipeline subobject type is not implemented");
    return Schemas[type];
}
