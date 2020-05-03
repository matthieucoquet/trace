#include "window.h"

#include <iostream>

constexpr bool verbose = false;
constexpr unsigned int initial_width = 1280u;
constexpr unsigned int initial_height = 720u;

//void framebuffer_resize_callback(GLFWwindow* window, int width, int height) 
//{
//    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
//    app->framebuffer_resize = true;
//    app->framebuffer_minimized = (width == 0 || height == 0);
//}

Window::Window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(initial_width, initial_height, "Vulkan Renderer", nullptr, nullptr);
    //glfwSetWindowUserPointer(m_window, this);
    //glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
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