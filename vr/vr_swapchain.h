#pragma once
#include "vr_common.h"

namespace vulkan
{
class Context;
}

namespace vr
{
class Session;
class Instance;

class Swapchain
{
public:
    static constexpr vk::Format required_format = vk::Format::eR8G8B8A8Unorm;

    xr::Swapchain swapchain;
    std::vector<xr::SwapchainImageVulkanKHR> images;
    std::vector<vk::Image> vk_images;
    std::vector<vk::ImageView> image_views;
    xr::Extent2Di view_extent;

    Swapchain() = delete;
    Swapchain(Instance& instance, xr::Session& session, vulkan::Context& context);
    Swapchain(xr::Session session, vulkan::Context& context, xr::Extent2Di extent);
    Swapchain(const Swapchain& other) = delete;
    Swapchain(Swapchain&& other) = delete;
    Swapchain& operator=(const Swapchain& other) = delete;
    Swapchain& operator=(Swapchain&& other) = delete;
    ~Swapchain();

    [[nodiscard]] uint32_t size() const noexcept { return static_cast<uint32_t>(images.size()); }
    [[nodiscard]] vk::Extent2D vk_view_extent() const {
        return vk::Extent2D(view_extent.width, view_extent.height);
    }
protected:
    vk::Device m_device;

    void create_image_views();
};

}
