#pragma once
#include "vk_common.h"
#include <array>

namespace vulkan
{

class Context;

class Desktop_swapchain
{
public:
    static constexpr vk::Format format{ vk::Format::eB8G8R8A8Srgb };
    static constexpr uint32_t image_count{ 3u };

    vk::SwapchainKHR swapchain;
    vk::Extent2D extent;
    std::array<vk::Image, image_count> images;

    Desktop_swapchain(Context& context);
    Desktop_swapchain(const Desktop_swapchain& other) = delete;
    Desktop_swapchain(Desktop_swapchain&& other) = delete;
    Desktop_swapchain& operator=(const Desktop_swapchain& other) = default;
    Desktop_swapchain& operator=(Desktop_swapchain&& other) = default;
    ~Desktop_swapchain();
private:
    vk::Device m_device;
};

}
