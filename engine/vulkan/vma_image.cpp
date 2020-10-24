#include "vma_image.hpp"
#include "vma_buffer.hpp"
#include <utility>

namespace vulkan
{

Vma_image::Vma_image(Vma_image&& other) noexcept :
    image(other.image),
    m_device(other.m_device),
    m_allocator(other.m_allocator),
    m_allocation(other.m_allocation)
{
    other.m_device = nullptr;
    other.image = nullptr;
    other.m_allocator = nullptr;
    other.m_allocation = nullptr;
}

Vma_image& Vma_image::operator=(Vma_image&& other) noexcept
{
    std::swap(m_device, other.m_device);
    std::swap(image, other.image);
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_allocation, other.m_allocation);
    return *this;
}

Vma_image::~Vma_image()
{
    if (m_device) {
        m_device.destroyImage(image);
        vmaFreeMemory(m_allocator, m_allocation);
    }
}

Vma_image::Vma_image(vk::Device device, VmaAllocator allocator, vk::ImageCreateInfo image_info, VmaMemoryUsage memory_usage) :
    m_device(device), m_allocator(allocator)
{
    const VkImageCreateInfo& c_image_info = image_info;
    VmaAllocationCreateInfo allocation_info{ .usage = memory_usage };

    VkImage c_image;
    [[maybe_unused]] VkResult result = vmaCreateImage(allocator, &c_image_info, &allocation_info, &c_image, &m_allocation, nullptr);
    assert(result == VK_SUCCESS);
    image = c_image;
}

Image_from_staged::Image_from_staged(vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::ImageCreateInfo image_info, const void* data, size_t size)
{
    staging = Vma_buffer(
        device, allocator,
        vk::BufferCreateInfo{
            .size = size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc },
        VMA_MEMORY_USAGE_CPU_ONLY);
    staging.map();
    staging.copy(data, size);
    staging.unmap();

    image_info.usage |= vk::ImageUsageFlagBits::eTransferDst;
    result = Vma_image(device, allocator, image_info, VMA_MEMORY_USAGE_GPU_ONLY);

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = {},
            .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = result.image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1 }
        });
    command_buffer.copyBufferToImage(staging.buffer, result.image, vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy{
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .layerCount = 1 },
            .imageExtent = image_info.extent });
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = result.image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1 }
        });
}

}