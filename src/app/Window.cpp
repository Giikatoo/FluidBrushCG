#include "app/Window.h"

#include "vulkan/VulkanCheck.h"

#include <cstdio>
#include <stdexcept>

namespace {

void onGlfwError(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

}  // namespace

Window::Window(int width, int height, const char* title) {
    glfwSetErrorCallback(onGlfwError);
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("glfwInit failed");
    }
    if (glfwVulkanSupported() != GLFW_TRUE) {
        glfwTerminate();
        throw std::runtime_error("GLFW could not find a Vulkan loader");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, onFramebufferResized);
}

Window::~Window() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_) == GLFW_TRUE; }

void Window::requestClose() { glfwSetWindowShouldClose(window_, GLFW_TRUE); }

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitEvents() const { glfwWaitEvents(); }

bool Window::isKeyPressed(int key) const { return glfwGetKey(window_, key) == GLFW_PRESS; }

VkExtent2D Window::framebufferExtent() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

bool Window::consumeResized() {
    const bool resized = resized_;
    resized_ = false;
    return resized;
}

std::vector<const char*> Window::requiredInstanceExtensions() {
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (extensions == nullptr) {
        throw std::runtime_error("GLFW reports no Vulkan surface instance extensions");
    }
    return {extensions, extensions + count};
}

VkSurfaceKHR Window::createSurface(VkInstance instance) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    vkCheck(glfwCreateWindowSurface(instance, window_, nullptr, &surface),
            "glfwCreateWindowSurface");
    return surface;
}

void Window::onFramebufferResized(GLFWwindow* window, int, int) {
    static_cast<Window*>(glfwGetWindowUserPointer(window))->resized_ = true;
}
