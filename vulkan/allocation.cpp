#include "allocation.h"
#include "command_buffer.h"
#include <utility>

namespace vulkan
{

Allocated_buffer::Allocated_buffer(Allocated_buffer&& other) noexcept :
    buffer(other.buffer),
    m_device(other.m_device),
    m_allocator(other.m_allocator),
    m_allocation(other.m_allocation)
{
    other.m_device = nullptr;
    other.buffer = nullptr;
    other.m_allocator = nullptr;
    other.m_allocation = nullptr;
}

Allocated_buffer& Allocated_buffer::operator=(Allocated_buffer&& other) noexcept
{
    std::swap(m_device, other.m_device);
    std::swap(buffer, other.buffer);
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_allocation, other.m_allocation);
    return *this;
}

Allocated_buffer::Allocated_buffer(vk::BufferCreateInfo buffer_info, VmaMemoryUsage memory_usage, vk::Device device, VmaAllocator allocator) :
    m_device(device), m_allocator(allocator)
{
    allocate(allocator, memory_usage, buffer_info);
}

Allocated_buffer::Allocated_buffer(vk::BufferCreateInfo buffer_info, const void* data,
    vk::Device device, VmaAllocator allocator, vk::CommandPool command_pool, vk::Queue queue) :
    m_device(device), m_allocator(allocator)
{
    Allocated_buffer staged_buffer(
        vk::BufferCreateInfo()
        .setSize(buffer_info.size)
        .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
        VMA_MEMORY_USAGE_CPU_ONLY,
        device, allocator);

    void* mapped;
    vmaMapMemory(allocator, staged_buffer.m_allocation, &mapped);
    memcpy(mapped, data, buffer_info.size);
    vmaUnmapMemory(allocator, staged_buffer.m_allocation);

    buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
    allocate(allocator, VMA_MEMORY_USAGE_GPU_ONLY, buffer_info);

    One_time_command_buffer command_buffer(device, command_pool, queue);
    command_buffer.command_buffer.copyBuffer(staged_buffer.buffer, buffer, vk::BufferCopy(0, 0, buffer_info.size));
    command_buffer.submit_and_wait_idle();
}

Allocated_buffer::~Allocated_buffer()
{
    if (m_device) {
        m_device.destroyBuffer(buffer);
        vmaFreeMemory(m_allocator, m_allocation);
    }
}

void Allocated_buffer::allocate(VmaAllocator allocator, VmaMemoryUsage memory_usage, vk::BufferCreateInfo buffer_info)
{
    const VkBufferCreateInfo& c_buffer_info = buffer_info;
    VmaAllocationCreateInfo allocation_info{ .usage = memory_usage };

    VkBuffer c_buffer;
    [[maybe_unused]] VkResult result = vmaCreateBuffer(allocator, &c_buffer_info, &allocation_info, &c_buffer, &m_allocation, nullptr);
    assert(result == VK_SUCCESS);
    buffer = c_buffer;
}

Allocated_image::Allocated_image(Allocated_image&& other) noexcept :
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

Allocated_image& Allocated_image::operator=(Allocated_image&& other) noexcept
{
    std::swap(m_device, other.m_device);
    std::swap(image, other.image);
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_allocation, other.m_allocation);
    return *this;
}

Allocated_image::~Allocated_image()
{
    if (m_device) {
        m_device.destroyImage(image);
        vmaFreeMemory(m_allocator, m_allocation);
    }
}

Allocated_image::Allocated_image(vk::ImageCreateInfo image_info, VmaMemoryUsage memory_usage, vk::Device device, VmaAllocator allocator) :
    m_device(device), m_allocator(allocator)
{
    const VkImageCreateInfo& c_image_info = image_info;
    VmaAllocationCreateInfo allocation_info{ .usage = memory_usage };

    VkImage c_image;
    [[maybe_unused]] VkResult result = vmaCreateImage(allocator, &c_image_info, &allocation_info, &c_image, &m_allocation, nullptr);
    assert(result == VK_SUCCESS);
    image = c_image;
}

}