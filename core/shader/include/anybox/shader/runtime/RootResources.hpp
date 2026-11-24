#pragma once

#include <anybox/shader/RootArgumentWriter.hpp>
#include <anybox/buffer/AddressSpace.hpp>
#include <array>

namespace AnyBox::Shader::Runtime {
class RootResources : public IRootResources {
public:
    RootResources(Graphics::Buffers::AddressSpace& buffers, Graphics::Buffers::AddressSpace& tables, std::uint64_t nullConstantAddress = 0) : buffers(buffers), tables(tables), nullConstantAddress(nullConstantAddress) {}
    RootBufferValue ResolveDescriptor(const BufferBinding& binding, std::uint64_t address) override;
    RootBufferValue ResolveTable(const BufferBinding& binding, std::uint32_t tableAddress, std::uint32_t descriptorOffset) override;
    std::span<const std::shared_ptr<const Graphics::Buffers::Mapping>> GetMappings() const noexcept { return std::span(mappings).first(count); }

private:
    static void checkBinding(const BufferBinding& binding);
    RootBufferValue resolveView(std::uint64_t address, std::uint64_t length);
    void retain(std::shared_ptr<const Graphics::Buffers::Mapping> mapping);
    Graphics::Buffers::AddressSpace& buffers;
    Graphics::Buffers::AddressSpace& tables;
    std::uint64_t nullConstantAddress;
    std::array<std::shared_ptr<const Graphics::Buffers::Mapping>, 2 * BufferArgumentPlan::UserRegisterCapacity> mappings;
    std::size_t count = 0;
};
}
