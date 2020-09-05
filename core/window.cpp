#include "window.hpp"

#include <fmt/core.h>

constexpr bool verbose = false;
constexpr unsigned int initial_height = 900;

Window::Window(float recommended_ratio)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(static_cast<int>(recommended_ratio * initial_height), initial_height, "Vulkan Renderer", nullptr, nullptr);
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

vk::SurfaceKHR Window::create_surface(vk::Instance instance) const
{
    VkSurfaceKHR c_surface;
    vk::Result result = static_cast<vk::Result>(glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &c_surface));
    return vk::createResultValue(result, c_surface, "glfwCreateWindowSurface");
}

bool Window::step()
{
    if (glfwWindowShouldClose(window))
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
        fmt::print("GLFW required extensions:\n");
        for (const auto& ext : extensions) {
            fmt::print("\t{}\n", ext);
        }
    }
    return extensions;
}