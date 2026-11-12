#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>

// Throws with the call description and numeric VkResult so failures carry context.
inline void vkCheck(VkResult result, const char* what) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(what) + " failed (VkResult " +
                                 std::to_string(static_cast<int>(result)) + ")");
    }
}
