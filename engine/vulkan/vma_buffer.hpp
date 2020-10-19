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
    Vma_buffer(vk::Device device, VmaAllocator allocator, vk::BufferCreateInfo buffer_info, VmaMemoryUsage memory_usage);
    ~Vma_buffer();

    void copy(const void* data, size_t size);
    void* map(); 
    void flush() {
        vmaFlushAllocation(m_allocator, m_allocation, 0, VK_WHOLE_SIZE);
    }
    void unmap();
private:
    vk::Device m_device;
    VmaAllocator m_allocator;
    VmaAllocation m_allocation{};
};

struct Buffer_from_staged
{
    [[nodiscard]] Buffer_from_staged(vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::BufferCreateInfo buffer_info, const void* data);
    Vma_buffer result;
    Vma_buffer staging;
};

}