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
    Vma_buffer(vk::BufferCreateInfo buffer_info, VmaMemoryUsage memory_usage, vk::Device device, VmaAllocator allocator);
    Vma_buffer(vk::BufferCreateInfo buffer_info, const void* data,
        vk::Device device, VmaAllocator allocator, vk::CommandPool command_pool, vk::Queue queue);
    ~Vma_buffer();

    void copy(const void* data, size_t size);
    void* map();
    void unmap();
private:
    vk::Device m_device;
    VmaAllocator m_allocator;
    VmaAllocation m_allocation{};

    void allocate(VmaAllocator allocator, VmaMemoryUsage memory_usage, vk::BufferCreateInfo buffer_info);
};

}