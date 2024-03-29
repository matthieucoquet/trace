#include "desktop_swapchain.hpp"
#include "context.hpp"

#include <algorithm>

namespace sdf_editor::vulkan
{

Desktop_swapchain::Desktop_swapchain(Context& context, bool vr_mode) :
    m_device(context.device)
{
    constexpr vk::ColorSpaceKHR colorspace{ vk::ColorSpaceKHR::eSrgbNonlinear };
    const vk::PresentModeKHR present_mode{ vr_mode ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo };

    auto available_formats = context.physical_device.getSurfaceFormatsKHR(context.surface);
    if (std::find(available_formats.cbegin(), available_formats.cend(), vk::SurfaceFormatKHR{ format, colorspace }) == available_formats.cend())
        throw std::runtime_error("Failed to find required surface format.");

    auto available_present_modes = context.physical_device.getSurfacePresentModesKHR(context.surface);
    if (std::find(available_present_modes.cbegin(), available_present_modes.cend(), present_mode) == available_present_modes.cend())
        throw std::runtime_error("Failed to find required surface present mode.");

    vk::SurfaceCapabilitiesKHR surface_capabilities = context.physical_device.getSurfaceCapabilitiesKHR(context.surface);
    if (surface_capabilities.currentExtent.width != UINT32_MAX) {
        extent = surface_capabilities.currentExtent;
    }
    else {
        assert(false); // Surface capabilities extend is set to UINT32_MAX, currently not supported
    }
    if (image_count < surface_capabilities.minImageCount && image_count > surface_capabilities.maxImageCount)
        throw std::runtime_error("Doesn't support required image count.");

    vk::SwapchainCreateInfoKHR swapchain_create_info {
        .surface = context.surface,
        .minImageCount = image_count,
        .imageFormat = format,
        .imageColorSpace = colorspace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = true };

    swapchain = m_device.createSwapchainKHR(swapchain_create_info);

    auto vec_images = m_device.getSwapchainImagesKHR(swapchain);
    for (size_t i = 0u; i < image_count; i++) {
        images[i] = vec_images[i];
    }
}

Desktop_swapchain::~Desktop_swapchain()
{
    m_device.destroySwapchainKHR(swapchain);
}

}
