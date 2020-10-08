#pragma once
#include "vk_common.hpp"

namespace vulkan
{

class Vma_image
{
public:
    vk::Image image;

    Vma_image() = default;
    Vma_image(const Vma_image& other) = delete;
    Vma_image(Vma_image&& other) noexcept;
    Vma_image& operator=(const Vma_image& other) = delete;
    Vma_image& operator=(Vma_image&& other) noexcept;
    Vma_image(vk::ImageCreateInfo image_info, VmaMemoryUsage memory_usage, vk::Device device, VmaAllocator allocator);
    Vma_image(vk::ImageCreateInfo image_info, const void* data, size_t size,
        vk::Device device, VmaAllocator allocator, vk::CommandPool command_pool, vk::Queue queue);
    ~Vma_image();
private:
    vk::Device m_device;
    VmaAllocator m_allocator{};
    VmaAllocation m_allocation{};
    void allocate(VmaAllocator allocator, VmaMemoryUsage memory_usage, vk::ImageCreateInfo image_info);
};

}