#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

// Owns the GLFW library lifetime and a single window without an OpenGL context.
class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void requestClose();
    void pollEvents() const;
    void waitEvents() const;
    bool isKeyPressed(int key) const;

    // Size in pixels; (0, 0) while minimized.
    VkExtent2D framebufferExtent() const;

    // True once after each framebuffer resize.
    bool consumeResized();

    static std::vector<const char*> requiredInstanceExtensions();
    VkSurfaceKHR createSurface(VkInstance instance) const;

private:
    static void onFramebufferResized(GLFWwindow* window, int width, int height);

    GLFWwindow* window_ = nullptr;
    bool resized_ = false;
};
