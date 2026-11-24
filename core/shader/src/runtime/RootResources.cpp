#include <anybox/shader/runtime/RootResources.hpp>
#include <algorithm>
#include <stdexcept>

using namespace AnyBox::Shader;
using Runtime::RootResources;

void RootResources::checkBinding(const BufferBinding& binding) {
    if (binding.hasCounter || binding.rasterizerOrdered) throw std::runtime_error("Counter and ordered buffer argument resolution is not implemented");
    if (binding.resourceClass == ResourceClass::ConstantBuffer) {
        if (binding.kind != BufferKind::Constant || binding.stride != 0) throw std::invalid_argument("Constant buffer argument binding is invalid");
    } else if (binding.resourceClass == ResourceClass::ShaderResource || binding.resourceClass == ResourceClass::UnorderedAccess) {
        if ((binding.kind != BufferKind::Raw && binding.kind != BufferKind::Structured) || (binding.kind == BufferKind::Raw && binding.stride != 0) || (binding.kind == BufferKind::Structured && binding.stride == 0)) throw std::invalid_argument("Shader buffer argument binding is invalid");
    } else throw std::invalid_argument("Shader argument is not a buffer resource");
}

void RootResources::retain(std::shared_ptr<const Graphics::Buffers::Mapping> mapping) {
    auto retained = GetMappings();
    if (std::ranges::find(retained, mapping) != retained.end()) return;
    if (count == mappings.size()) throw std::length_error("Compute root resource retention exceeds its ABI capacity");
    mappings[count++] = std::move(mapping);
}

RootBufferValue RootResources::resolveView(std::uint64_t address, std::uint64_t length) {
    if (length == 0) return {0, 0};
    if (address == 0) throw std::invalid_argument("Nonempty shader buffer view has a null address");
    auto mapping = buffers.Resolve(address, length);
    retain(std::move(mapping));
    return {address, length};
}

RootBufferValue RootResources::ResolveDescriptor(const BufferBinding& binding, std::uint64_t address) {
    checkBinding(binding);
    if (address == 0) return {0, 0};
    auto mapping = buffers.Resolve(address);
    auto length = mapping->GetSize() - (address - mapping->GetGuestAddress());
    if (binding.kind == BufferKind::Constant) length = std::min(length, static_cast<std::uint64_t>(binding.byteSize));
    retain(std::move(mapping));
    return {address, length};
}
