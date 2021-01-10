#include "vma_buffer.hpp"
#include <utility>

namespace vulkan
{

Vma_buffer::Vma_buffer(Vma_buffer&& other) noexcept :
    buffer(std::move(other.buffer)),
    m_device(other.m_device),
    m_allocator(other.m_allocator),
    m_allocation(other.m_allocation),
    m_mapped(other.m_mapped)
{
    other.m_device = nullptr;
    other.buffer = nullptr;
    other.m_allocator = nullptr;
    other.m_allocation = nullptr;
    other.m_mapped = nullptr;
}

Vma_buffer& Vma_buffer::operator=(Vma_buffer&& other) noexcept
{
    std::swap(m_device, other.m_device);
    std::swap(buffer, other.buffer);
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_allocation, other.m_allocation);
    std::swap(m_mapped, other.m_mapped);
    return *this;
}

Vma_buffer::Vma_buffer(vk::Device device, VmaAllocator allocator, vk::BufferCreateInfo buffer_info, VmaAllocationCreateInfo allocation_info) :
    m_device(device), m_allocator(allocator)
{
    const VkBufferCreateInfo& c_buffer_info = buffer_info;
    VkBuffer c_buffer;
    [[maybe_unused]] VkResult result = vmaCreateBuffer(allocator, &c_buffer_info, &allocation_info, &c_buffer, &m_allocation, nullptr);
    assert(result == VK_SUCCESS);
    buffer = c_buffer;
    if (allocation_info.flags == VMA_ALLOCATION_CREATE_MAPPED_BIT) {
        VmaAllocationInfo info{};
        vmaGetAllocationInfo(m_allocator, m_allocation, &info);
        m_mapped = info.pMappedData;
    }
}


Vma_buffer::~Vma_buffer()
{
    free();
}

Buffer_from_staged::Buffer_from_staged(vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::BufferCreateInfo buffer_info, const void* data)
{
    staging = Vma_buffer(
        device, allocator,
        vk::BufferCreateInfo{
            .size = buffer_info.size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc },
            VmaAllocationCreateInfo{ .usage = VMA_MEMORY_USAGE_CPU_ONLY });
    staging.map();
    staging.copy(data, buffer_info.size);
    staging.unmap();

    buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
    result = Vma_buffer(device, allocator, buffer_info, VmaAllocationCreateInfo{ .usage = VMA_MEMORY_USAGE_GPU_ONLY });

    command_buffer.copyBuffer(staging.buffer, result.buffer, vk::BufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = buffer_info.size });
}

}