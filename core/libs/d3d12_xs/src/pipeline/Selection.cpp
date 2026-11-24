#include "Offline.hpp"
#include <anybox/graphics/container/View.hpp>
#include <anybox/graphics/container/Dxil.hpp>

bool AnyBox::Graphics::RequiresPreparedCompute(Device& device, std::span<const std::byte> code) {
    Container::DxbcView container(code);
    auto program = container.FindPart(0x4C495844);
    if (program && Container::DxilView(program->data).HasEmbeddedData()) return true;
    auto features = container.FindPart(0x30494653);
    if (!features) return false;
    if (features->data.size() != 8) throw std::invalid_argument("Compute shader feature information size is invalid");
    std::uint64_t flags = 0;
    for (unsigned index = 0; index < 8; ++index) flags |= std::to_integer<std::uint64_t>(features->data[index]) << (index * 8);
    constexpr std::uint64_t native16ShaderFlag = 0x40000;
    if ((flags & native16ShaderFlag) == 0) return false;
    D3D12_FEATURE_DATA_D3D12_OPTIONS4 options{};
    CheckGraphicsResult(device.GetHost()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &options, sizeof(options)), "Query native 16-bit shader support");
    return options.Native16BitShaderOpsSupported == FALSE;
}
