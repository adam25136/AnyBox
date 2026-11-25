#include "StreamFixture.hpp"

void AnyBox::Graphics::Tests::CheckPipelineStreamParser() {
    constexpr std::array<std::size_t, 25> sizes{16, 24, 24, 24, 24, 24, 24, 40, 336, 8, 48, 56, 24, 8, 8, 40, 8, 16, 8, 24, 8, 64, 32, 24, 24};
    for (std::uint32_t type = 0; type < sizes.size(); ++type) {
        auto schema = GetPipelineSubobjectSchema(type);
        Require(schema.size == sizes[type], "Xbox pipeline stream subobject size changed");
        std::vector<std::uint64_t> words(schema.size / 8);
        std::memcpy(words.data(), &type, sizeof(type));
        auto bytes = std::as_bytes(std::span(words));
        PipelineStream parsed(bytes);
        Require(parsed.Has(type), "Pipeline stream lost a present subobject");
        Reject<std::invalid_argument>([&] { PipelineStream invalid(bytes.first(bytes.size() - 1)); });
        auto duplicate = words;
        duplicate.insert(duplicate.end(), words.begin(), words.end());
        Reject<std::invalid_argument>([&] { PipelineStream invalid(std::as_bytes(std::span(duplicate))); });
    }
    StreamFixture graphics;
    std::array<std::byte, 4> code{};
    auto shader = D3D12_SHADER_BYTECODE{code.data(), code.size()};
    graphics.Append(0, static_cast<ID3D12RootSignature*>(nullptr));
    graphics.Append(1, shader);
    graphics.Append(2, shader);
    graphics.Append(3, D3D12_SHADER_BYTECODE{});
    graphics.Append(4, D3D12_SHADER_BYTECODE{});
    graphics.Append(5, D3D12_SHADER_BYTECODE{});
    graphics.Append(8, D3D12_BLEND_DESC{});
    graphics.Append(9, UINT32_MAX);
    graphics.Append(10, D3D12_RASTERIZER_DESC{});
    graphics.Append(21, D3D12_DEPTH_STENCIL_DESC1{});
    graphics.Append(17, DXGI_SAMPLE_DESC{1, 0});
    graphics.Append(12, D3D12_INPUT_LAYOUT_DESC{});
    graphics.Append(14, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    graphics.Append(15, D3D12_RT_FORMAT_ARRAY{});
    graphics.Append(16, DXGI_FORMAT_UNKNOWN);
    Require(graphics.GetBytes().size() == 688, "Synthetic graphics pipeline stream ABI size changed");
    PipelineStream parsed(graphics.GetBytes());
    Require(parsed.GetKind() == PipelineStreamKind::Graphics && parsed.GetShader(1).BytecodeLength == code.size(), "Graphics pipeline stages were not retained");
    Reject<std::invalid_argument>([&] { parsed.GetComputeDescription(); });
    Reject<std::logic_error>([&] { parsed.Read<std::uint64_t>(9); });
    Reject<std::invalid_argument>([&] { parsed.GetShader(9); });
    StreamFixture mesh;
    mesh.Append(23, shader);
    mesh.Append(24, shader);
    Require(PipelineStream(mesh.GetBytes()).GetKind() == PipelineStreamKind::Mesh, "Xbox amplification and mesh stage numbering changed");
    mesh.Append(1, shader);
    Reject<std::invalid_argument>([&] { PipelineStream(mesh.GetBytes()).GetKind(); });
    StreamFixture compute;
    compute.Append(6, shader);
    compute.Append(9, UINT32_MAX);
    Reject<std::runtime_error>([&] { PipelineStream(compute.GetBytes()).GetComputeDescription(); });
    compute.Append(1, shader);
    Reject<std::invalid_argument>([&] { PipelineStream(compute.GetBytes()).GetKind(); });
    StreamFixture mismatch;
    mismatch.Append(1, D3D12_SHADER_BYTECODE{nullptr, 4});
    Reject<std::invalid_argument>([&] { PipelineStream(mismatch.GetBytes()).GetKind(); });
    StreamFixture tessellation;
    tessellation.Append(1, shader);
    tessellation.Append(4, shader);
    Reject<std::invalid_argument>([&] { PipelineStream(tessellation.GetBytes()).GetKind(); });
    Reject<std::invalid_argument>([&] { PipelineStream empty({}); });
    Reject<std::invalid_argument>([&] { PipelineStream misaligned(graphics.GetBytes().subspan(1)); });
    Reject<std::runtime_error>([&] { GetPipelineSubobjectSchema(25); });
    StreamFixture depth;
    depth.Append(11, D3D12_DEPTH_STENCIL_DESC{});
    depth.Append(21, D3D12_DEPTH_STENCIL_DESC1{});
    Reject<std::invalid_argument>([&] { PipelineStream invalid(depth.GetBytes()); });
}
