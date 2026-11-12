#include "app/Application.h"

#include "vulkan/VulkanCheck.h"

#include <cstdio>

namespace {

constexpr int kInitialWidth = 1280;
constexpr int kInitialHeight = 720;

// Linear values; the sRGB swapchain format encodes them on write.
constexpr VkClearColorValue kClearColor = {{0.02F, 0.03F, 0.05F, 1.0F}};

VkImageMemoryBarrier2 colorImageBarrier(VkImage image, VkImageLayout oldLayout,
                                        VkImageLayout newLayout,
                                        VkPipelineStageFlags2 srcStage,
                                        VkAccessFlags2 srcAccess,
                                        VkPipelineStageFlags2 dstStage,
                                        VkAccessFlags2 dstAccess) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    return barrier;
}

void cmdImageBarrier(VkCommandBuffer commandBuffer, const VkImageMemoryBarrier2& barrier) {
    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(commandBuffer, &dependency);
}

}  // namespace

Application::Application(const ApplicationOptions& options)
    : options_(options),
      window_(kInitialWidth, kInitialHeight, "FluidBrushCG"),
      context_(window_, options.validation),
      swapchain_(context_, window_) {
    try {
        createFrameResources();
        createRenderFinishedSemaphores();
    } catch (...) {
        destroyRenderFinishedSemaphores();
        destroyFrameResources();
        throw;
    }

    std::printf("GPU: %s\nValidation: %s\nSwapchain: %ux%u, %u images\n",
                context_.deviceName().c_str(), context_.validationEnabled() ? "on" : "off",
                swapchain_.extent().width, swapchain_.extent().height,
                swapchain_.imageCount());
}

Application::~Application() {
    vkDeviceWaitIdle(context_.device());
    destroyRenderFinishedSemaphores();
    destroyFrameResources();
}

void Application::run() {
    uint64_t presented = 0;
    while (!window_.shouldClose()) {
        window_.pollEvents();
        if (window_.isKeyPressed(GLFW_KEY_ESCAPE)) {
            window_.requestClose();
        }
        if (drawFrame()) {
            ++presented;
            if (options_.maxFrames != 0 && presented >= options_.maxFrames) {
                break;
            }
        }
    }
    vkDeviceWaitIdle(context_.device());
}

void Application::createFrameResources() {
    const VkDevice device = context_.device();
    for (FrameResources& frame : frames_) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = context_.queueFamily();
        vkCheck(vkCreateCommandPool(device, &poolInfo, nullptr, &frame.commandPool),
                "vkCreateCommandPool");

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = frame.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkCheck(vkAllocateCommandBuffers(device, &allocInfo, &frame.commandBuffer),
                "vkAllocateCommandBuffers");

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCheck(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.imageAvailable),
                "vkCreateSemaphore");

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // First frame must not block.
        vkCheck(vkCreateFence(device, &fenceInfo, nullptr, &frame.inFlight), "vkCreateFence");
    }
}

void Application::createRenderFinishedSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < swapchain_.imageCount(); ++i) {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        vkCheck(vkCreateSemaphore(context_.device(), &semaphoreInfo, nullptr, &semaphore),
                "vkCreateSemaphore");
        renderFinished_.push_back(semaphore);
    }
}

void Application::destroyRenderFinishedSemaphores() noexcept {
    for (VkSemaphore semaphore : renderFinished_) {
        vkDestroySemaphore(context_.device(), semaphore, nullptr);
    }
    renderFinished_.clear();
}

void Application::destroyFrameResources() noexcept {
    const VkDevice device = context_.device();
    for (FrameResources& frame : frames_) {
        vkDestroyFence(device, frame.inFlight, nullptr);
        vkDestroySemaphore(device, frame.imageAvailable, nullptr);
        // Destroying the pool also frees its command buffer.
        vkDestroyCommandPool(device, frame.commandPool, nullptr);
        frame = {};
    }
}

bool Application::drawFrame() {
    const VkExtent2D framebuffer = window_.framebufferExtent();
    if (framebuffer.width == 0 || framebuffer.height == 0) {
        window_.waitEvents();  // Minimized: sleep until the window changes.
        return false;
    }
    if (window_.consumeResized() && !recreateSwapchain()) {
        return false;
    }

    const VkDevice device = context_.device();
    FrameResources& frame = frames_[frameIndex_];
    vkCheck(vkWaitForFences(device, 1, &frame.inFlight, VK_TRUE, UINT64_MAX), "vkWaitForFences");

    uint32_t imageIndex = 0;
    const VkResult acquire = vkAcquireNextImageKHR(device, swapchain_.handle(), UINT64_MAX,
                                                   frame.imageAvailable, VK_NULL_HANDLE,
                                                   &imageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return false;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        vkCheck(acquire, "vkAcquireNextImageKHR");
    }

    // Reset only after a successful acquire so a skipped frame cannot leave the fence unsignaled.
    vkCheck(vkResetFences(device, 1, &frame.inFlight), "vkResetFences");
    vkCheck(vkResetCommandPool(device, frame.commandPool, 0), "vkResetCommandPool");
    recordCommands(frame.commandBuffer, imageIndex);

    VkSemaphoreSubmitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo.semaphore = frame.imageAvailable;
    waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkCommandBufferSubmitInfo commandInfo{};
    commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandInfo.commandBuffer = frame.commandBuffer;

    VkSemaphoreSubmitInfo signalInfo{};
    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo.semaphore = renderFinished_[imageIndex];
    signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;
    vkCheck(vkQueueSubmit2(context_.queue(), 1, &submitInfo, frame.inFlight), "vkQueueSubmit2");

    const VkSwapchainKHR swapchain = swapchain_.handle();
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinished_[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    const VkResult present = vkQueuePresentKHR(context_.queue(), &presentInfo);

    frameIndex_ = (frameIndex_ + 1) % kFramesInFlight;

    if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else {
        vkCheck(present, "vkQueuePresentKHR");
    }
    return true;
}

void Application::recordCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex) const {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");

    const VkImage image = swapchain_.image(imageIndex);

    // Source stage matches the acquire semaphore's wait stage, so the transition
    // does not start before the image is available. Contents are cleared, so UNDEFINED is safe.
    cmdImageBarrier(commandBuffer,
                    colorImageBarrier(image, VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                                      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT));

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchain_.imageView(imageIndex);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = kClearColor;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, swapchain_.extent()};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    // Future passes (marble field, brush mixing, present) are recorded here.
    vkCmdEndRendering(commandBuffer);

    cmdImageBarrier(commandBuffer,
                    colorImageBarrier(image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_2_NONE, 0));

    vkCheck(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");
}

bool Application::recreateSwapchain() {
    VkExtent2D framebuffer = window_.framebufferExtent();
    while ((framebuffer.width == 0 || framebuffer.height == 0) && !window_.shouldClose()) {
        window_.waitEvents();
        framebuffer = window_.framebufferExtent();
    }
    if (framebuffer.width == 0 || framebuffer.height == 0) {
        return false;
    }

    vkDeviceWaitIdle(context_.device());
    swapchain_.recreate();
    // Recreated rather than reused: a failed present can leave a semaphore signaled.
    destroyRenderFinishedSemaphores();
    createRenderFinishedSemaphores();
    window_.consumeResized();
    return true;
}
