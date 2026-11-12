#include "vulkan/Swapchain.h"

#include "app/Window.h"
#include "vulkan/VulkanCheck.h"
#include "vulkan/VulkanContext.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {

// sRGB so shader/clear values are linear and encoded by the hardware on write.
VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (VkFormat preferred : {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB}) {
        for (const VkSurfaceFormatKHR& format : formats) {
            if (format.format == preferred &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
    }
    return formats.front();
}

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, VkExtent2D framebuffer) {
    if (caps.currentExtent.width != UINT32_MAX) {
        return caps.currentExtent;
    }
    return {std::clamp(framebuffer.width, caps.minImageExtent.width, caps.maxImageExtent.width),
            std::clamp(framebuffer.height, caps.minImageExtent.height,
                       caps.maxImageExtent.height)};
}

}  // namespace

Swapchain::Swapchain(const VulkanContext& context, const Window& window)
    : context_(context), window_(window) {
    create();
}

Swapchain::~Swapchain() {
    destroyViews();
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(context_.device(), swapchain_, nullptr);
    }
}

void Swapchain::recreate() { create(); }

void Swapchain::create() {
    const VkPhysicalDevice physicalDevice = context_.physicalDevice();
    const VkSurfaceKHR surface = context_.surface();

    VkSurfaceCapabilitiesKHR caps{};
    vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps),
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount == 0) {
        throw std::runtime_error("Surface reports no formats");
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
    const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);

    const VkExtent2D extent = chooseExtent(caps, window_.framebufferExtent());
    if (extent.width == 0 || extent.height == 0) {
        throw std::runtime_error("Cannot create a swapchain with a zero-sized surface");
    }

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0) {
        imageCount = std::min(imageCount, caps.maxImageCount);
    }

    const VkCompositeAlphaFlagBitsKHR compositeAlpha =
        (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
            ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
            : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = compositeAlpha;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;  // Always available; vsynced.
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = swapchain_;

    VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
    vkCheck(vkCreateSwapchainKHR(context_.device(), &createInfo, nullptr, &newSwapchain),
            "vkCreateSwapchainKHR");

    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    try {
        uint32_t actualCount = 0;
        vkCheck(vkGetSwapchainImagesKHR(context_.device(), newSwapchain, &actualCount, nullptr),
                "vkGetSwapchainImagesKHR");
        images.resize(actualCount);
        vkCheck(vkGetSwapchainImagesKHR(context_.device(), newSwapchain, &actualCount,
                                        images.data()),
                "vkGetSwapchainImagesKHR");

        for (VkImage image : images) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = surfaceFormat.format;
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            VkImageView view = VK_NULL_HANDLE;
            vkCheck(vkCreateImageView(context_.device(), &viewInfo, nullptr, &view),
                    "vkCreateImageView");
            views.push_back(view);
        }
    } catch (...) {
        for (VkImageView view : views) {
            vkDestroyImageView(context_.device(), view, nullptr);
        }
        vkDestroySwapchainKHR(context_.device(), newSwapchain, nullptr);
        throw;
    }

    destroyViews();
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(context_.device(), swapchain_, nullptr);
    }
    swapchain_ = newSwapchain;
    format_ = surfaceFormat.format;
    extent_ = extent;
    images_ = std::move(images);
    views_ = std::move(views);
}

void Swapchain::destroyViews() noexcept {
    for (VkImageView view : views_) {
        vkDestroyImageView(context_.device(), view, nullptr);
    }
    views_.clear();
    images_.clear();
}
