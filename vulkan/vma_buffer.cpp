#include "vma_buffer.hpp"
#include "command_buffer.hpp"
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

Vma_buffer::Vma_buffer(vk::BufferCreateInfo buffer_info, VmaMemoryUsage memory_usage, vk::Device device, VmaAllocator allocator) :
    m_device(device), m_allocator(allocator)
{
    allocate(allocator, memory_usage, buffer_info);
}

Vma_buffer::Vma_buffer(vk::BufferCreateInfo buffer_info, const void* data,
    vk::Device device, VmaAllocator allocator, vk::CommandPool command_pool, vk::Queue queue) :
    m_device(device), m_allocator(allocator)
{
    Vma_buffer staged_buffer(
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
    command_buffer.command_buffer.copyBuffer(staged_buffer.buffer, buffer, vk::BufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = buffer_info.size });
    command_buffer.submit_and_wait_idle();
}

Vma_buffer::~Vma_buffer()
{
    if (m_device) {
        m_device.destroyBuffer(buffer);
        vmaFreeMemory(m_allocator, m_allocation);
    }
}

void Vma_buffer::allocate(VmaAllocator allocator, VmaMemoryUsage memory_usage, vk::BufferCreateInfo buffer_info)
{
    const VkBufferCreateInfo& c_buffer_info = buffer_info;
    VmaAllocationCreateInfo allocation_info{ .usage = memory_usage };

    VkBuffer c_buffer;
    [[maybe_unused]] VkResult result = vmaCreateBuffer(allocator, &c_buffer_info, &allocation_info, &c_buffer, &m_allocation, nullptr);
    assert(result == VK_SUCCESS);
    buffer = c_buffer;
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

}