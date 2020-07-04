#pragma once
#include "vulkan/vk_common.h"

class Window
{
public:
    GLFWwindow* window;
    bool framebuffer_resize = false;
    bool framebuffer_minimized = false;

    Window(float recommended_ratio);
    Window(const Window& other) = delete;
    Window(Window&& other) = delete;
    Window& operator=(const Window& other) = delete;
    Window& operator=(Window&& other) = delete;
    ~Window();

    [[nodiscard]] vk::SurfaceKHR create_surface(vk::Instance instance) const;
    [[nodiscard]] std::vector<const char*> required_extensions() const;
    [[nodiscard]] bool step();
};