#include "texture.hpp"
#include "context.hpp"
#include "command_buffer.hpp"
#include "vma_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace sdf_editor::vulkan
{

Texture::Texture(Texture&& other) noexcept :
    height(other.height),
    width(other.width),
    image(std::move(other.image)),
    image_view(other.image_view),
    m_device(other.m_device)
{
    other.m_device = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    std::swap(m_device, other.m_device);
    height = other.height;
    width = other.width;
    image = std::move(other.image);
    std::swap(image_view, other.image_view);
    return *this;
}

Texture::Texture(Context& context, std::string_view filename) :
    m_device(context.device)
{
    int tex_channels;
    stbi_uc* data = stbi_load(filename.data(), &height, &width, &tex_channels, STBI_rgb_alpha);

    if (!data) {
        throw std::runtime_error("Loading texture failed.");
    }

    One_time_command_buffer command_buffer(context.device, context.command_pool, context.graphics_queue);
    Image_from_staged image_and_staged (
        m_device, context.allocator, command_buffer.command_buffer,
        vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
            .mipLevels = 1u,
            .arrayLayers = 1u,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined
        },
        data, static_cast<size_t>(height * width * 4));
    stbi_image_free(data);
    image = std::move(image_and_staged.result);
    command_buffer.submit_and_wait_idle();

    image_view = m_device.createImageView(vk::ImageViewCreateInfo{
        .image = image.image,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Unorm,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0u,
            .levelCount = 1u,
            .baseArrayLayer = 0u,
            .layerCount = 1u
        }
    });
}


Texture::Texture(Context& context, vk::Extent2D extent, vk::CommandBuffer command_buffer) :
    height(extent.height),
    width(extent.width),
    m_device(context.device)
{
    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    image = Vma_image(
        m_device, context.allocator,
        vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = {extent.width, extent.height, 1},
            .mipLevels = 1u,
            .arrayLayers = 1u,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined
        },
        VMA_MEMORY_USAGE_GPU_ONLY);
    image_view = m_device.createImageView(
        vk::ImageViewCreateInfo{
           .image = image.image,
           .viewType = vk::ImageViewType::e2D,
           .format = format,
           .subresourceRange = {
               .aspectMask = vk::ImageAspectFlagBits::eColor,
               .baseMipLevel = 0u,
               .levelCount = 1u,
               .baseArrayLayer = 0u,
               .layerCount = 1u} });
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = {},
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image.image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1
            } });
}

Texture::~Texture()
{
    if (m_device) {
        m_device.destroyImageView(image_view);
    }
}

Sampler::Sampler(Context& context) :
    m_device(context.device)
{
    sampler = m_device.createSampler(vk::SamplerCreateInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = false,
        //.maxAnisotropy = 16.0f,
        .compareEnable = false,
        //.compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 1.0f,
        .unnormalizedCoordinates = false
        });
}

Sampler::~Sampler()
{
    m_device.destroySampler(sampler);
}

}