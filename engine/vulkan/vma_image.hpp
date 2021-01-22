#pragma once
#include "vk_common.hpp"
#include "vma_buffer.hpp"

namespace sdf_editor::vulkan
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
    Vma_image(vk::Device device, VmaAllocator allocator, vk::ImageCreateInfo image_info, VmaMemoryUsage memory_usage);
    ~Vma_image();
private:
    vk::Device m_device;
    VmaAllocator m_allocator{};
    VmaAllocation m_allocation{};
};

struct Image_from_staged
{
    [[nodiscard]] Image_from_staged(vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::ImageCreateInfo image_info, const void* data, size_t size);
    Vma_image result;
    Vma_buffer staging;
};

}