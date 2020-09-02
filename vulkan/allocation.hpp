#pragma once
#include "vk_common.hpp"

namespace vulkan
{

class Allocated_buffer
{
public:
    vk::Buffer buffer;

    Allocated_buffer() = default;
    Allocated_buffer(const Allocated_buffer& other) = delete;
    Allocated_buffer(Allocated_buffer&& other) noexcept;
    Allocated_buffer& operator=(const Allocated_buffer& other) = delete;
    Allocated_buffer& operator=(Allocated_buffer&& other) noexcept;
    Allocated_buffer(vk::BufferCreateInfo buffer_info, VmaMemoryUsage memory_usage, vk::Device device, VmaAllocator allocator);
    Allocated_buffer(vk::BufferCreateInfo buffer_info, const void* data,
        vk::Device device, VmaAllocator allocator, vk::CommandPool command_pool, vk::Queue queue);
    ~Allocated_buffer();

    void copy(const void* data, size_t size);
    void* map();
    void unmap();
private:
    vk::Device m_device;
    VmaAllocator m_allocator;
    VmaAllocation m_allocation{};

    void allocate(VmaAllocator allocator, VmaMemoryUsage memory_usage, vk::BufferCreateInfo buffer_info);
};

class Allocated_image
{
public:
    vk::Image image;

    Allocated_image() = default;
    Allocated_image(const Allocated_image& other) = delete;
    Allocated_image(Allocated_image&& other) noexcept;
    Allocated_image& operator=(const Allocated_buffer& other) = delete;
    Allocated_image& operator=(Allocated_image&& other) noexcept;
    Allocated_image(vk::ImageCreateInfo image_info, VmaMemoryUsage memory_usage, vk::Device device, VmaAllocator allocator);
    Allocated_image(vk::ImageCreateInfo image_info, const void* data, size_t size,
        vk::Device device, VmaAllocator allocator, vk::CommandPool command_pool, vk::Queue queue);
    ~Allocated_image();
private:
    vk::Device m_device;
    VmaAllocator m_allocator{};
    VmaAllocation m_allocation{};
    void allocate(VmaAllocator allocator, VmaMemoryUsage memory_usage, vk::ImageCreateInfo image_info);
};

}