#include "vr_swapchain.hpp"
#include "vulkan/context.hpp"
#include "instance.hpp"
#include "session.hpp"

#include <algorithm>
#include <fmt/core.h>

namespace sdf_editor::vr
{

constexpr bool verbose = true;

Swapchain::Swapchain(Instance& instance, xr::Session& session, vulkan::Context& context) :
    m_device(context.device)
{
    std::vector<int64_t> supported_formats = session.enumerateSwapchainFormatsToVector();
    if constexpr (verbose) {
        fmt::print("Supported Vulkan format in the XR runtime:\n");
        for (auto format : supported_formats) {
            fmt::print("\t{}\n", vk::to_string(static_cast<vk::Format>(format)));
        }
    }
    if (std::none_of(supported_formats.cbegin(), supported_formats.cend(), [](const auto& format) {
        return static_cast<int64_t>(required_format) == format;
    })) {
        throw std::runtime_error("Required format not supported by OpenXR runtime.");
    }

    auto view_configuration_views = instance.instance.enumerateViewConfigurationViewsToVector(instance.system_id, xr::ViewConfigurationType::PrimaryStereo);
    xr::ViewConfigurationView view_configuration_view = view_configuration_views[0];
    assert(view_configuration_views[0].recommendedImageRectHeight == view_configuration_views[1].recommendedImageRectHeight);
    assert(view_configuration_views[0].recommendedImageRectWidth == view_configuration_views[1].recommendedImageRectWidth);
    assert(view_configuration_views[0].recommendedSwapchainSampleCount == view_configuration_views[1].recommendedSwapchainSampleCount);
    if constexpr (verbose) {
        fmt::print("XR runtime view configuration views:\n");
        for (const auto& view : view_configuration_views) {
            fmt::print("\tRecommended: {}x{} {} samples\n", view.recommendedImageRectWidth, view.recommendedImageRectHeight, view.recommendedSwapchainSampleCount);
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
    images = swapchain.enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>();
    vk_images.reserve(images.size());
    for (const auto& image : images) {
        vk_images.push_back(image.image);
    }
    create_image_views();
}

/*Swapchain::Swapchain(xr::Session session, vulkan::Context& context, xr::Extent2Di extent) :
    view_extent(extent),
    m_device(context.device)
{
    std::vector<int64_t> supported_formats = session.enumerateSwapchainFormatsToVector();
    if (std::none_of(supported_formats.cbegin(), supported_formats.cend(), [](const auto& format) {
        return static_cast<int64_t>(required_format) == format;
    })) {
        throw std::runtime_error("Required format not supported by OpenXR runtime.");
    }

    swapchain = session.createSwapchain(xr::SwapchainCreateInfo{
        .createFlags = xr::SwapchainCreateFlagBits::None,
        .usageFlags = xr::SwapchainUsageFlagBits::ColorAttachment, // OpenXR doesn't expose Storage bit so we have to first render to another image and copy
        .format = static_cast<int64_t>(required_format),
        .sampleCount = 1u, //3u,
        .width = static_cast<uint32_t>(extent.width),
        .height = static_cast<uint32_t>(extent.height),
        .faceCount = 1,
        .arraySize = 1,
        .mipCount = 1 });
    images = swapchain.enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>();
    vk_images.reserve(images.size());
    for (const auto& image : images) {
        vk_images.push_back(image.image);
    }
    create_image_views();
}*/

Swapchain::~Swapchain()
{
    for (auto image_view : image_views) {
        m_device.destroyImageView(image_view);
    }
}

void Swapchain::create_image_views()
{
    for (auto& image : images) {
        image_views.push_back(m_device.createImageView(vk::ImageViewCreateInfo{
            .image = image.image,
            .viewType = vk::ImageViewType::e2D,
            .format = required_format,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            }
        }));
    }
}

}