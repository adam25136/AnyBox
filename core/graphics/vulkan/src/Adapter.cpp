#include <anybox/vulkan/Context.hpp>
#include <cstring>

void AnyBox::Vulkan::Context::findAdapter(const AdapterSelection& selection) {
    std::uint32_t count = 0;
    Check(instanceFunctions.enumeratePhysicalDevices(instance, &count, nullptr), "Count Vulkan adapters");
    if (count == 0) throw std::runtime_error("No Vulkan adapters are available");
    std::vector<VkPhysicalDevice> devices(count);
    Check(instanceFunctions.enumeratePhysicalDevices(instance, &count, devices.data()), "Enumerate Vulkan adapters");
    devices.resize(count);
    if (auto index = std::get_if<std::uint32_t>(&selection)) {
        if (*index >= devices.size()) throw std::out_of_range("Vulkan adapter index is unavailable");
        physicalDevice = devices[*index];
        return;
    }

    const auto& expected = std::get<AdapterLuid>(selection);
    for (auto candidate : devices) {
        VkPhysicalDeviceIDProperties identity{};
        identity.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        VkPhysicalDeviceProperties2 physical{};
        physical.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physical.pNext = &identity;
        instanceFunctions.getPhysicalDeviceProperties(candidate, &physical);
        if (!identity.deviceLUIDValid || std::memcmp(identity.deviceLUID, expected.data(), expected.size()) != 0) continue;
        if (physicalDevice) throw std::runtime_error("Multiple Vulkan adapters have the requested Windows LUID");
        physicalDevice = candidate;
    }
    if (!physicalDevice) throw AdapterNotFound("No Vulkan adapter has the requested Windows LUID");
}
