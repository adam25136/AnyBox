#include "Description.hpp"

AnyBox::Graphics::PipelineStream::PipelineStream(std::span<const std::byte> bytes) {
    if (bytes.empty() || reinterpret_cast<std::uintptr_t>(bytes.data()) % alignof(void*) != 0) throw std::invalid_argument("Xbox pipeline stream is empty or misaligned");
    while (!bytes.empty()) {
        if (bytes.size() < sizeof(std::uint32_t)) throw std::invalid_argument("Xbox pipeline stream subobject type is truncated");
        std::uint32_t type;
        std::memcpy(&type, bytes.data(), sizeof(type));
        auto schema = GetPipelineSubobjectSchema(type);
        if (schema.size > bytes.size()) throw std::invalid_argument("Xbox pipeline stream subobject payload is truncated");
        if (Has(type)) throw std::invalid_argument("Xbox pipeline stream contains a duplicate subobject");
        objects[type] = bytes.subspan(schema.offset, schema.payload);
        bytes = bytes.subspan(schema.size);
    }
    if (Has(11) && Has(21)) throw std::invalid_argument("Xbox pipeline stream contains both depth stencil descriptions");
}

D3D12_SHADER_BYTECODE AnyBox::Graphics::PipelineStream::GetShader(std::uint32_t type) const {
    if ((type < 1 || type > 6) && type != 23 && type != 24) throw std::invalid_argument("Pipeline subobject is not a shader stage");
    if (!Has(type)) return {};
    auto shader = Read<D3D12_SHADER_BYTECODE>(type);
    if ((shader.pShaderBytecode == nullptr) != (shader.BytecodeLength == 0)) throw std::invalid_argument("Xbox pipeline shader pointer and bytecode size disagree");
    return shader;
}

AnyBox::Graphics::PipelineStreamKind AnyBox::Graphics::PipelineStream::GetKind() const {
    std::array<bool, 25> active{};
    for (auto type : {1u, 2u, 3u, 4u, 5u, 6u, 23u, 24u}) active[type] = GetShader(type).BytecodeLength != 0;
    if (active[6]) {
        for (auto type : {1u, 2u, 3u, 4u, 5u, 23u, 24u}) if (active[type]) throw std::invalid_argument("Compute and graphics shader stages cannot share a pipeline");
        return PipelineStreamKind::Compute;
    }
    if (active[24]) {
        if (active[1] || active[3] || active[4] || active[5]) throw std::invalid_argument("Mesh and vertex pipeline stages cannot be combined");
        return PipelineStreamKind::Mesh;
    }
    if (active[23] || !active[1] || active[3] != active[4]) throw std::invalid_argument("Xbox graphics pipeline shader stage combination is invalid");
    return PipelineStreamKind::Graphics;
}
