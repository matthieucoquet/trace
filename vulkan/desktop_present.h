#pragma once
#include "vk_common.h"

#include "desktop_swapchain.h"
//#include "allocation.h"

namespace vulkan
{

class Context;

class Desktop_present
{
public:

    static constexpr uint32_t max_frames_in_flight{ Swapchain::image_count };

    Desktop_present(Context& context, Scene& scene);
    Desktop_present(const Desktop_present& other) = delete;
    Desktop_present(Desktop_present&& other) = delete;
    Desktop_present& operator=(const Desktop_present& other) = default;
    Desktop_present& operator=(Desktop_present&& other) = default;
    ~Desktop_present();

    //void reload_pipeline(Context& context);

    void present();
private:
    vk::Device m_device;
    vk::Queue m_queue;
    //vk::CommandPool m_command_pool;
    Desktop_swapchain m_swapchain;

    //std::array<vk::CommandBuffer, Swapchain::image_count> m_command_buffers;

    //uint32_t m_current_frame = 0u;
    std::vector<vk::Semaphore> m_semaphore_available;
    std::vector<vk::Semaphore> m_semaphore_finished;
    //std::array<vk::Fence, max_frames_in_flight> m_fence_in_flight;
    //std::array<vk::Fence, max_frames_in_flight> m_fence_swapchain_in_flight;

    void create_command_buffers();
    void create_synchronization();
};

}*/