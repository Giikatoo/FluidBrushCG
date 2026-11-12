#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class VulkanContext;
class Window;

// Owns the swapchain and one image view per swapchain image.
class Swapchain {
public:
    Swapchain(const VulkanContext& context, const Window& window);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // The caller must ensure the GPU no longer uses the old images, and that the
    // window's framebuffer size is non-zero.
    void recreate();

    VkSwapchainKHR handle() const { return swapchain_; }
    VkFormat format() const { return format_; }
    VkExtent2D extent() const { return extent_; }
    uint32_t imageCount() const { return static_cast<uint32_t>(images_.size()); }
    VkImage image(uint32_t index) const { return images_[index]; }
    VkImageView imageView(uint32_t index) const { return views_[index]; }

private:
    void create();
    void destroyViews() noexcept;

    const VulkanContext& context_;
    const Window& window_;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;
};
