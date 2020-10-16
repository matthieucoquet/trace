#include "vma_buffer.hpp"
#include <utility>

namespace vulkan
{

Vma_buffer::Vma_buffer(Vma_buffer&& other) noexcept :
    buffer(std::move(other.buffer)),
    m_device(other.m_device),
    m_allocator(other.m_allocator),
    m_allocation(other.m_allocation)
{
    other.m_device = nullptr;
    other.buffer = nullptr;
    other.m_allocator = nullptr;
    other.m_allocation = nullptr;
}

Vma_buffer& Vma_buffer::operator=(Vma_buffer&& other) noexcept
{
    std::swap(m_device, other.m_device);
    std::swap(buffer, other.buffer);
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_allocation, other.m_allocation);
    return *this;
}

Vma_buffer::Vma_buffer(vk::Device device, VmaAllocator allocator, vk::BufferCreateInfo buffer_info, VmaMemoryUsage memory_usage) :
    m_device(device), m_allocator(allocator)
{
    const VkBufferCreateInfo& c_buffer_info = buffer_info;
    VmaAllocationCreateInfo allocation_info{ .usage = memory_usage };

    VkBuffer c_buffer;
    [[maybe_unused]] VkResult result = vmaCreateBuffer(allocator, &c_buffer_info, &allocation_info, &c_buffer, &m_allocation, nullptr);
    assert(result == VK_SUCCESS);
    buffer = c_buffer;
}


Vma_buffer::~Vma_buffer()
{
    if (m_device) {
        m_device.destroyBuffer(buffer);
        vmaFreeMemory(m_allocator, m_allocation);
    }
}

void Vma_buffer::copy(const void* data, size_t size)
{
    void* mapped;
    vmaMapMemory(m_allocator, m_allocation, &mapped);
    memcpy(mapped, data, size);
    vmaFlushAllocation(m_allocator, m_allocation, 0, VK_WHOLE_SIZE);
    vmaUnmapMemory(m_allocator, m_allocation);
}

void* Vma_buffer::map()
{
    void* mapped;
    vmaMapMemory(m_allocator, m_allocation, &mapped);
    return mapped;
}

void Vma_buffer::unmap()
{
    vmaFlushAllocation(m_allocator, m_allocation, 0, VK_WHOLE_SIZE);
    vmaUnmapMemory(m_allocator, m_allocation);
}

Buffer_from_staged::Buffer_from_staged(vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::BufferCreateInfo buffer_info, const void* data)
{
    staging = Vma_buffer(
        device, allocator,
        vk::BufferCreateInfo{
            .size = buffer_info.size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc },
        VMA_MEMORY_USAGE_CPU_ONLY);
    staging.copy(data, buffer_info.size);

    buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
    result = Vma_buffer(device, allocator, buffer_info, VMA_MEMORY_USAGE_GPU_ONLY);

    command_buffer.copyBuffer(staging.buffer, result.buffer, vk::BufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = buffer_info.size });
}

}