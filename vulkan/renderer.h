#pragma once
#include "vk_common.h"

#include "raytracing_pipeline.h"
#include "scene.h"
#include "acceleration_structure.h"
#include "allocation.h"

namespace vulkan
{

class Context;

class Renderer
{
public:
    std::vector<Allocated_image> storage_images;

    Renderer(Context& context, Scene& scene);
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = default;
    Renderer& operator=(Renderer&& other) = default;
    ~Renderer();

    void reload_pipeline(Context& context);

    void update_uniforms(Scene& scene, uint32_t swapchain_index);
    void start_recording(vk::CommandBuffer command_buffer, vk::Image swapchain_image, uint32_t swapchain_id, vk::Extent2D extent);
    void end_recording(vk::CommandBuffer command_buffer, vk::Image swapchain_image, uint32_t swapchain_id);

    void create_uniforms(Context& context, uint32_t swapchain_size);
    // OpenXR doesn't expose Storage bit so we have to first render to another image and copy
    void create_storage_image(Context& context, vk::Extent2D extent, vk::Format format, uint32_t swapchain_size);
    void create_descriptor_sets(const Scene& scene, uint32_t swapchain_size);
    void create_synchronization();
private:
    vk::Device m_device;
    vk::Queue m_queue;
    vk::CommandPool m_command_pool;
    Raytracing_pipeline m_pipeline;
    Blas m_blas;
    Tlas m_tlas;

    std::vector<vk::CommandBuffer> m_command_buffers;
    std::vector<Allocated_buffer> m_scene_uniforms;
    std::vector<vk::DescriptorSet> m_descriptor_sets;

    std::vector<vk::ImageView> m_image_views;

    //vk::ImageView m_image_view;

    vk::DescriptorPool m_descriptor_pool;

    vk::Fence m_render_fence;
    /*uint32_t m_current_frame = 0u;
    std::array<vk::Semaphore, max_frames_in_flight> m_semaphore_available_in_flight;
    std::array<vk::Semaphore, max_frames_in_flight> m_semaphore_finished_in_flight;
    std::array<vk::Fence, max_frames_in_flight> m_fence_in_flight;
    std::array<vk::Fence, max_frames_in_flight> m_fence_swapchain_in_flight;*/
};

}