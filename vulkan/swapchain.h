#pragma once
#include "common.h"
#include <array>

namespace vulkan
{

class Context;

class Swapchain
{
public:
    static constexpr vk::Format format{ vk::Format::eB8G8R8A8Srgb };
    static constexpr uint32_t image_count{ 3u };

    vk::SwapchainKHR swapchain;
    vk::Extent2D extent;
    std::array<vk::Image, image_count> images;
    std::array<vk::ImageView, image_count> image_views;

    Swapchain(Context& context);
    Swapchain(const Swapchain& other) = delete;
    Swapchain(Swapchain&& other) = delete;
    Swapchain& operator=(const Swapchain& other) = default;
    Swapchain& operator=(Swapchain&& other) = default;
    ~Swapchain();
private:
    vk::Device m_device;

    void create_image_views();
};

}
