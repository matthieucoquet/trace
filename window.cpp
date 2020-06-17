#include "window.h"

#include <iostream>

constexpr bool verbose = false;
//constexpr unsigned int initial_width = 1800u;
constexpr unsigned int initial_height = 900;

Window::Window(void* user_pointer, GLFWkeyfun keyboard_callback, float recommended_ratio)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(static_cast<int>(recommended_ratio * initial_height), initial_height, "Vulkan Renderer", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, user_pointer);
    glfwSetKeyCallback(m_window, keyboard_callback);
}

Window::~Window()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

vk::SurfaceKHR Window::create_surface(vk::Instance instance) const
{
    VkSurfaceKHR c_surface;
    vk::Result result = static_cast<vk::Result>(glfwCreateWindowSurface(static_cast<VkInstance>(instance), m_window, nullptr, &c_surface));
    return vk::createResultValue(result, c_surface, "glfwCreateWindowSurface");
}

bool Window::step()
{
    if (glfwWindowShouldClose(m_window))
        return false;
    glfwPollEvents();
    return true;
}

std::vector<const char*> Window::required_extensions() const
{
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if constexpr (verbose)
    {
        std::cout << "GLFW required extensions:" << std::endl;
        for (const auto& ext : extensions) {
            std::cout << "\t" << ext << std::endl;
        }
    }
    return extensions;
}