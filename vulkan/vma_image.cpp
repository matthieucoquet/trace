#include "vma_image.hpp"
#include "vma_buffer.hpp"
#include "command_buffer.hpp"
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

Vma_image::Vma_image(vk::ImageCreateInfo image_info, VmaMemoryUsage memory_usage, vk::Device device, VmaAllocator allocator) :
    m_device(device), m_allocator(allocator)
{
    allocate(allocator, memory_usage, image_info);
}

Vma_image::Vma_image(vk::ImageCreateInfo image_info, const void* data, size_t size,
    vk::Device device, VmaAllocator allocator, vk::CommandPool command_pool, vk::Queue queue) :
    m_device(device), m_allocator(allocator)
{
    Vma_buffer staged_buffer(
        vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
        VMA_MEMORY_USAGE_CPU_ONLY,
        device, allocator);
    staged_buffer.copy(data, size);

    allocate(allocator, VMA_MEMORY_USAGE_GPU_ONLY, image_info);

    One_time_command_buffer command_buffer(device, command_pool, queue);
    command_buffer.command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        {}, {}, {},
        vk::ImageMemoryBarrier()
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(image)
        .setSrcAccessMask({})
        .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setSubresourceRange(vk::ImageSubresourceRange()
            .setLevelCount(1u)
            .setLayerCount(1)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setAspectMask(vk::ImageAspectFlagBits::eColor)));
    command_buffer.command_buffer.copyBufferToImage(staged_buffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, vk::BufferImageCopy()
        .setImageExtent(image_info.extent)
        .setImageSubresource(vk::ImageSubresourceLayers().setLayerCount(1).setAspectMask(vk::ImageAspectFlagBits::eColor)));
    command_buffer.command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            {}, {}, {},
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask({})
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setLevelCount(1u)
                .setLayerCount(1)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor)));
    command_buffer.submit_and_wait_idle();
}

void Vma_image::allocate(VmaAllocator allocator, VmaMemoryUsage memory_usage, vk::ImageCreateInfo image_info)
{
    const VkImageCreateInfo& c_image_info = image_info;
    VmaAllocationCreateInfo allocation_info{ .usage = memory_usage };

    VkImage c_image;
    [[maybe_unused]] VkResult result = vmaCreateImage(allocator, &c_image_info, &allocation_info, &c_image, &m_allocation, nullptr);
    assert(result == VK_SUCCESS);
    image = c_image;
}

}