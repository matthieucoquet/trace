#pragma once
#include "vk_common.hpp"

namespace vulkan
{

class Vma_buffer
{
public:
    vk::Buffer buffer;

    Vma_buffer() = default;
    Vma_buffer(const Vma_buffer& other) = delete;
    Vma_buffer(Vma_buffer&& other) noexcept;
    Vma_buffer& operator=(const Vma_buffer& other) = delete;
    Vma_buffer& operator=(Vma_buffer&& other) noexcept;
    Vma_buffer(vk::Device device, VmaAllocator allocator, vk::BufferCreateInfo buffer_info, VmaAllocationCreateInfo allocation_info);
    ~Vma_buffer();

    void copy(const void* data, size_t size);
    void flush();
    void* map();
    void unmap();
    void free();
private:
    vk::Device m_device;
    VmaAllocator m_allocator;
    VmaAllocation m_allocation{};
    void* m_mapped = nullptr;
};

struct Buffer_from_staged
{
    [[nodiscard]] Buffer_from_staged(vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::BufferCreateInfo buffer_info, const void* data);
    Vma_buffer result;
    Vma_buffer staging;
};

inline void Vma_buffer::copy(const void* data, size_t size)
{
    memcpy(m_mapped, data, size);
}

inline void Vma_buffer::flush()
{
    vmaFlushAllocation(m_allocator, m_allocation, 0, VK_WHOLE_SIZE);
}

inline void* Vma_buffer::map()
{
    vmaMapMemory(m_allocator, m_allocation, &m_mapped);
    return m_mapped;
}

inline void Vma_buffer::unmap()
{
    vmaFlushAllocation(m_allocator, m_allocation, 0, VK_WHOLE_SIZE);
    vmaUnmapMemory(m_allocator, m_allocation);
    m_mapped = nullptr;
}

inline void Vma_buffer::free()
{
    if (m_device) {
        m_device.destroyBuffer(buffer);
        vmaFreeMemory(m_allocator, m_allocation);
        m_device = nullptr;
    }
}

}