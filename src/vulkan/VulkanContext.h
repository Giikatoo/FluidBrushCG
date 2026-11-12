#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

class Window;

// Owns the instance, debug messenger, surface, physical-device choice, logical
// device, and the single graphics+present queue.
class VulkanContext {
public:
    VulkanContext(const Window& window, bool enableValidation);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    VkInstance instance() const { return instance_; }
    VkPhysicalDevice physicalDevice() const { return physicalDevice_; }
    VkDevice device() const { return device_; }
    VkSurfaceKHR surface() const { return surface_; }
    VkQueue queue() const { return queue_; }
    uint32_t queueFamily() const { return queueFamily_; }
    const std::string& deviceName() const { return deviceName_; }
    bool validationEnabled() const { return debugMessenger_ != VK_NULL_HANDLE; }

private:
    void createInstance(const Window& window, bool enableValidation);
    void pickPhysicalDevice();
    void createDevice();
    void destroy() noexcept;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    uint32_t queueFamily_ = 0;
    std::string deviceName_;
};
