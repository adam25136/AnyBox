#include <anybox/shader/runtime/ComputeMemory.hpp>
#include <utility>
#include <cstring>

using namespace AnyBox::Shader::Runtime;

ComputeMemory::ComputeMemory(AnyBox::Vulkan::Context& context, MemoryTable table, FaultLifetime faultLifetime) : context(context), table(std::move(table)), clearOnBegin(faultLifetime == FaultLifetime::Submission) {
    if (faultLifetime != FaultLifetime::Submission && faultLifetime != FaultLifetime::Recording) throw std::invalid_argument("Compute fault lifetime is invalid");
    auto limit = context.GetProperties().limits.maxStorageBufferRange;
    if (this->table.GetByteSize() > limit || ShaderRecompiler::BdaAbi::FaultBufferBytes > limit) throw std::length_error("Compute metadata exceeds the Vulkan storage buffer range limit");
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    pages = std::make_unique<Vulkan::Buffer>(Vulkan::Buffer::Allocate(context, this->table.GetByteSize(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, properties));
    faults = std::make_unique<Vulkan::Buffer>(Vulkan::Buffer::Allocate(context, ShaderRecompiler::BdaAbi::FaultBufferBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, properties));
    this->table.Write(pages->Map());
    auto initialized = faults->Map();
    std::memset(initialized.data(), 0, initialized.size());
}
