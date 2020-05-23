#pragma once
#include "vulkan/common.h"

class Window
{
public:
    bool framebuffer_resize = false;
    bool framebuffer_minimized = false;

    Window(void* user_pointer, GLFWkeyfun keyboard_callback);
    Window(const Window& other) = delete;
    Window(Window&& other) = delete;
    Window& operator=(const Window& other) = delete;
    Window& operator=(Window&& other) = delete;
    ~Window();

    [[nodiscard]] vk::SurfaceKHR create_surface(vk::Instance instance) const;
    [[nodiscard]] std::vector<const char*> required_extensions() const;
    [[nodiscard]] bool step();
private:
    GLFWwindow* m_window;
};