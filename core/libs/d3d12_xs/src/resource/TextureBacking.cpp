#include "Texture.hpp"
#include <utility>

AnyBox::Graphics::TextureBacking::TextureBacking(Device& device, ResourceMemory memory, const Vulkan::ImageDescription& image, const Descriptor::Texture2DDescription& description, const D3D12_HEAP_PROPERTIES& properties, D3D12_HEAP_FLAGS flags, D3D12_BARRIER_LAYOUT layout, const Descriptor::SurfacePitch& pitch) : memory(std::move(memory)), allocation(device.GetTextureAddresses(), device.GetGpuContext(), image, description, reinterpret_cast<std::uintptr_t>(this->memory.GetAddress()), this->memory.GetSize()), properties(properties), flags(flags), initialLayout(layout), pitch(pitch) {}
