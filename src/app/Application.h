#pragma once

#include "app/Window.h"
#include "vulkan/Swapchain.h"
#include "vulkan/VulkanContext.h"

#include <array>
#include <cstdint>
#include <vector>

struct ApplicationOptions {
    // Stop after this many presented frames; 0 runs until the window closes.
    uint64_t maxFrames = 0;
    bool validation = false;
};

// Owns the window, Vulkan context, swapchain, and per-frame synchronization, and
// runs the frame loop. The skeleton only clears the swapchain image.
class Application {
public:
    explicit Application(const ApplicationOptions& options);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    static constexpr uint32_t kFramesInFlight = 2;

    struct FrameResources {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkFence inFlight = VK_NULL_HANDLE;
    };

    void createFrameResources();
    void createRenderFinishedSemaphores();
    void destroyRenderFinishedSemaphores() noexcept;
    void destroyFrameResources() noexcept;

    // Returns false when nothing was presented (minimized window or stale swapchain).
    bool drawFrame();
    void recordCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
    bool recreateSwapchain();

    ApplicationOptions options_;
    Window window_;
    VulkanContext context_;
    Swapchain swapchain_;

    std::array<FrameResources, kFramesInFlight> frames_{};
    // One per swapchain image: presentation may still hold the semaphore after the
    // frame's fence signals, so it cannot be tied to the frame-in-flight index.
    std::vector<VkSemaphore> renderFinished_;
    uint32_t frameIndex_ = 0;
};
