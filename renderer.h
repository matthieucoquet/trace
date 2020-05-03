#pragma once
#include "common.h"

#include "swapchain.h"
#include "raytracing_pipeline.h"
#include "scene.h"
#include "acceleration_structure.h"
#include "allocation.h"

class Context;

class Renderer
{
public:

    static constexpr uint32_t max_frames_in_flight{ Swapchain::image_count };

    Renderer(Context& context, Scene& scene);
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = default;
    Renderer& operator=(Renderer&& other) = default;
    ~Renderer();

    void step();
private:
    vk::Device m_device;
    vk::Queue m_queue;
    Swapchain m_swapchain;
    Raytracing_pipeline m_pipeline;
    Blas m_blas;
    Tlas m_tlas;

    std::array<vk::CommandBuffer, Swapchain::image_count> m_command_buffers;

    Allocated_image m_storage_image; 
    vk::ImageView m_image_view;

    vk::DescriptorPool m_descriptor_pool;
    std::vector<vk::DescriptorSet> m_descriptor_sets;

    uint32_t m_current_frame = 0u;
    std::array<vk::Semaphore, max_frames_in_flight> m_semaphore_available_in_flight;
    std::array<vk::Semaphore, max_frames_in_flight> m_semaphore_finished_in_flight;
    std::array<vk::Fence, max_frames_in_flight> m_fence_in_flight;
    std::array<vk::Fence, max_frames_in_flight> m_fence_swapchain_in_flight;

    void create_command_buffers(Context& context);
    void create_storage_image(Context& context); // TODO do we need storage image
    void create_descriptor_sets(Scene& scene);
    void create_synchronization();
};
