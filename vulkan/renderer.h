#pragma once
#include "vk_common.h"

#include "raytracing_pipeline.h"
#include "acceleration_structure.h"
#include "allocation.h"
#include "core/scene.h"

namespace vulkan
{

class Context;

struct Per_frame
{
    Tlas tlas;
    Allocated_buffer scene_uniform;
    Allocated_buffer primitives;
    Allocated_image storage_image;
    vk::ImageView image_view;
};

class Renderer
{
public:
    std::vector<Per_frame> per_frame;

    Renderer(Context& context, Scene& scene);
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = default;
    Renderer& operator=(Renderer&& other) = default;
    ~Renderer();

    void reload_pipeline(Context& context);

    void update_per_frame_data(Scene& scene, uint32_t swapchain_index);
    void start_recording(vk::CommandBuffer command_buffer, Scene& scene, vk::Image swapchain_image, uint32_t swapchain_id, vk::Extent2D extent);
    void end_recording(vk::CommandBuffer command_buffer, vk::Image swapchain_image, uint32_t swapchain_id);

    void create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, vk::Format format, uint32_t swapchain_size);
    // OpenXR doesn't expose Storage bit so we have to first render to another image and copy
    void create_descriptor_sets(vk::DescriptorPool descriptor_pool, uint32_t swapchain_size);
private:
    vk::Device m_device;
    vk::Queue m_queue;
    vk::CommandPool m_command_pool;
    Raytracing_pipeline m_pipeline;
    Blas m_blas;

    std::vector<vk::DescriptorSet> m_descriptor_sets;
};

}