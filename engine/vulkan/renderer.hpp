#pragma once
#include "vk_common.hpp"

#include "raytracing_pipeline.hpp"
#include "acceleration_structure.hpp"
#include "vma_buffer.hpp"
#include "vma_image.hpp"
#include "texture.hpp"
#include "core/scene.hpp"

namespace vulkan
{

class Context;

struct Per_frame
{
    Tlas tlas;
    Vma_buffer materials;
    Vma_buffer lights;
    Vma_image storage_image;
    vk::ImageView image_view;
};

class Renderer
{
public:
    std::vector<Per_frame> per_frame;

    Renderer(Context& context, Scene& scene);
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;
    ~Renderer();

    void update_per_frame_data(Scene& scene, size_t command_pool_id);

    void start_recording(vk::CommandBuffer command_buffer, Scene& scene, size_t command_pool_id);
    void barrier_vr_swapchain(vk::CommandBuffer command_buffer, vk::Image swapchain_image);
    void trace(vk::CommandBuffer command_buffer, Scene& scene, size_t command_pool_id, vk::Extent2D extent);
    void copy_to_vr_swapchain(vk::CommandBuffer command_buffer, vk::Image swapchain_image, size_t command_pool_id, vk::Extent2D extent);
    void end_recording(vk::CommandBuffer command_buffer, size_t command_pool_id);

    void create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, vk::Format format, size_t command_pool_size);
    // OpenXR doesn't expose Storage bit so we have to first render to another image and copy
    void create_descriptor_sets(vk::DescriptorPool descriptor_pool, size_t command_pool_size);
private:
    vk::Device m_device;
    VmaAllocator m_allocator;
    vk::Queue m_queue;
    Texture m_noise_texture;
    Raytracing_pipeline m_pipeline;
    Blas m_blas;

    Vma_buffer staging;

    std::vector<vk::DescriptorSet> m_descriptor_sets;
};

}
