cmake_minimum_required(VERSION 3.24)
project(FluidBrushCG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan 1.3 REQUIRED)

# GLFW is fetched at configure time so the project has no machine-specific paths.
include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

add_executable(FluidBrushCG
    src/main.cpp
    src/app/Application.cpp
    src/app/Window.cpp
    src/vulkan/Swapchain.cpp
    src/vulkan/VulkanContext.cpp
)
target_include_directories(FluidBrushCG PRIVATE src)
target_link_libraries(FluidBrushCG PRIVATE Vulkan::Vulkan glfw)

if(MSVC)
    target_compile_options(FluidBrushCG PRIVATE /W4 /permissive-)
else()
    target_compile_options(FluidBrushCG PRIVATE -Wall -Wextra -Wpedantic)
endif()
