#include "CommittedBuffers.hpp"

void AnyBox::Graphics::Tests::CheckCommittedBuffers(Device* device) {
    for (auto type : {D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_TYPE_READBACK}) {
        for (auto size : {1u, 8u, 65535u, 65536u, 65537u}) {
            for (bool enhanced : {false, true}) {
                auto resource = MakeCommittedBuffer(device, type, size, enhanced);
                auto address = Method<decltype(&Resource::GetGPUVirtualAddress)>(resource.get(), 11)(resource.get());
                Require(address != 0 && address % 65536 == 0, "Committed buffer GPU address is invalid");
                Require(resource->GetMetadata().Width == size && resource->GetGpuBuffer().GetSize() == size, "Committed buffer logical size changed");
                D3D12_HEAP_PROPERTIES properties{};
                D3D12_HEAP_FLAGS flags{};
                auto heap = Method<decltype(&Resource::GetHeapProperties)>(resource.get(), 14);
                Require(heap(resource.get(), &properties, &flags) == S_OK && properties.Type == type && properties.CreationNodeMask == 1 && properties.VisibleNodeMask == 1 && flags == D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, "Committed buffer heap properties changed");
                Require(heap(resource.get(), nullptr, &flags) == S_OK, "Committed buffer optional heap property output failed");
                auto map = Method<decltype(&Resource::Map)>(resource.get(), 8);
                void* mapped = resource.get();
                if (type == D3D12_HEAP_TYPE_DEFAULT) {
                    Reject<std::runtime_error>([&] { map(resource.get(), 0, nullptr, &mapped); });
                    Require(!mapped, "Rejected default buffer mapping retained a stale pointer");
                    continue;
                }
                D3D12_RANGE range{0, size};
                Require(map(resource.get(), 0, &range, &mapped) == S_OK && reinterpret_cast<std::uintptr_t>(mapped) == address, "Committed buffer mapping is not its shared allocation");
                MEMORY_BASIC_INFORMATION memory{};
                Require(VirtualQuery(mapped, &memory, sizeof(memory)) != 0 && memory.State == MEM_COMMIT, "Committed buffer allocation is not committed");
                Require(((memory.Protect & PAGE_WRITECOMBINE) != 0) == (type == D3D12_HEAP_TYPE_UPLOAD), "Committed buffer CPU cache policy disagrees with its heap type");
                Method<decltype(&Resource::Unmap)>(resource.get(), 9)(resource.get(), 0, nullptr);
                range.End = size + 1;
                Reject<std::invalid_argument>([&] { map(resource.get(), 0, &range, &mapped); });
                Reject<std::invalid_argument>([&] { map(resource.get(), 1, nullptr, &mapped); });
            }
        }
    }
}
