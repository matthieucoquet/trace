#include "vr_swapchain.h"
//#include "vr_context.h"
#include "vulkan/context.h"
#include "instance.h"
#include "session.h"

#include <algorithm>
#include <iostream>

constexpr bool verbose = true;

namespace vr
{

Swapchain& Swapchain::operator=(Swapchain&& other)
{
    if (swapchain) {
        swapchain.destroy();
        for (auto image_view : image_views) {
            m_device.destroyImageView(image_view);
        }
    }
    swapchain = std::move(other.swapchain);
    images = std::move(other.images);
    image_views = std::move(other.image_views);
    view_extent = other.view_extent;
    other.swapchain = nullptr;
    other.images.clear();
    other.image_views.clear();
    return *this;
}

Swapchain::Swapchain(Instance& instance, xr::Session& session, vulkan::Context& context) :
    m_device(context.device)
{
    std::vector<int64_t> supported_formats = session.enumerateSwapchainFormats();
    if (std::none_of(supported_formats.cbegin(), supported_formats.cend(), [](const auto& format) {
        return static_cast<int64_t>(required_format) == format;
    })) {
        throw std::runtime_error("Required format not supported by OpenXR runtime.");
    }
    if constexpr (verbose) {
        std::cout << "Supported Vulkan format in the XR runtime:" << std::endl;
        for (auto format : supported_formats) {
            std::cout << "\t" << vk::to_string(static_cast<vk::Format>(format)) << std::endl;
        }
    }

    auto view_configuration_views = instance.instance.enumerateViewConfigurationViews(instance.system_id, xr::ViewConfigurationType::PrimaryStereo);
    xr::ViewConfigurationView view_configuration_view = view_configuration_views[0];
    assert(view_configuration_views[0].recommendedImageRectHeight == view_configuration_views[1].recommendedImageRectHeight);
    assert(view_configuration_views[0].recommendedImageRectWidth == view_configuration_views[1].recommendedImageRectWidth);
    assert(view_configuration_views[0].recommendedSwapchainSampleCount == view_configuration_views[1].recommendedSwapchainSampleCount);
    if constexpr (verbose) {
        std::cout << "XR runtime view configuration views:" << std::endl;
        for (auto view : view_configuration_views) {
            std::cout << "\t" << "Recommended: "
                << view.recommendedImageRectWidth << " x "
                << view.recommendedImageRectHeight << " "
                << view.recommendedSwapchainSampleCount << " samples " << std::endl;
        }
    }

    view_extent.height = view_configuration_view.recommendedImageRectHeight;
    view_extent.width = view_configuration_view.recommendedImageRectWidth;
    swapchain = session.createSwapchain(xr::SwapchainCreateInfo{
        .createFlags = xr::SwapchainCreateFlagBits::None,
        .usageFlags = xr::SwapchainUsageFlagBits::TransferDst, // OpenXR doesn't expose Storage bit so we have to first render to another image and copy
        .format = static_cast<int64_t>(required_format),
        .sampleCount = view_configuration_view.recommendedSwapchainSampleCount,
        .width = view_configuration_view.recommendedImageRectWidth * 2u,  // One swapchain of double width
        .height = view_configuration_view.recommendedImageRectHeight,
        .faceCount = 1,
        .arraySize = 1,
        .mipCount = 1 });
    images = swapchain.enumerateSwapchainImages<xr::SwapchainImageVulkanKHR>();
    create_image_views();
}

Swapchain::~Swapchain()
{
    if (swapchain) {
        swapchain.destroy(); // good place ?
    }
    for (auto image_view : image_views) {
        m_device.destroyImageView(image_view);
    }
}

void Swapchain::create_image_views()
{
    for (auto& image : images) {
        image_views.push_back(m_device.createImageView(vk::ImageViewCreateInfo()
            .setImage(image.image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(required_format)
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseMipLevel(0u)
                .setLevelCount(1u)
                .setBaseArrayLayer(0u)
                .setLayerCount(1u))));
    }
}

std::vector<vk::Image> Swapchain::get_vk_images() const
{
    std::vector<vk::Image> vk_images;
    vk_images.reserve(images.size());
    for (const auto& image : images) {
        vk_images.push_back(image.image);
    }
    return vk_images;
}

}