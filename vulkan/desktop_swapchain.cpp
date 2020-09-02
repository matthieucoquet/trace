#include "desktop_swapchain.hpp"
#include "context.hpp"

#include <algorithm>

namespace vulkan
{

Desktop_swapchain::Desktop_swapchain(Context& context) :
    m_device(context.device)
{
    constexpr vk::ColorSpaceKHR colorspace{ vk::ColorSpaceKHR::eSrgbNonlinear };
    constexpr vk::PresentModeKHR present_mode{ vk::PresentModeKHR::eMailbox };

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

    auto swapchain_create_info = vk::SwapchainCreateInfoKHR()
        .setSurface(context.surface)
        .setMinImageCount(image_count)
        .setImageFormat(format)
        .setImageColorSpace(colorspace)
        .setPresentMode(present_mode)
        .setImageExtent(extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eTransferDst)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setPreTransform(surface_capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(true);

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