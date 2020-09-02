#pragma once
#include "vk_common.hpp"
#include "desktop_swapchain.hpp"

namespace vulkan
{

class Context;

class Desktop_mirror
{
public:
    Desktop_mirror(Context& context, size_t size);
    Desktop_mirror(const Desktop_mirror& other) = delete;
    Desktop_mirror(Desktop_mirror&& other) = delete;
    Desktop_mirror& operator=(const Desktop_mirror& other) = default;
    Desktop_mirror& operator=(Desktop_mirror&& other) = default;
    ~Desktop_mirror();

    void create_synchronization(size_t size);

    void copy(vk::CommandBuffer& command_buffer, vk::Image vr_image, size_t current_id, vk::Extent2D extent);
    void present(vk::CommandBuffer& command_buffer, vk::Fence fence, size_t current_id);
private:
    vk::Device m_device;
    vk::Queue m_queue;
    Desktop_swapchain m_swapchain;
    uint32_t m_image_id;

    std::vector<vk::Semaphore> m_semaphore_available;
    std::vector<vk::Semaphore> m_semaphore_finished;

};

}