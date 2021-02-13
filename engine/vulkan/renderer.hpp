#pragma once
#include "vk_common.hpp"

#include "raytracing_pipeline.hpp"
#include "acceleration_structure.hpp"
#include "vma_buffer.hpp"
#include "vma_image.hpp"
#include "texture.hpp"
#include "imgui_render.hpp"
#include "core/scene.hpp"

namespace sdf_editor::vulkan
{

class Context;

struct Per_frame
{
    std::vector<Blas> characters_blas;
    Tlas tlas;
    Vma_buffer materials;
    Vma_buffer lights;
    Vma_image storage_image;
    vk::ImageView image_view;
};

class Renderer
{
public:
    //static constexpr vk::Format storage_format = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format storage_format = vk::Format::eR16G16B16A16Sfloat;
    std::vector<Per_frame> per_frame;

    Renderer(Context& context, Scene& scene, size_t command_pool_size);
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;
    ~Renderer();

    void update_per_frame_data(Scene& scene, size_t command_pool_id);

    void start_recording(vk::CommandBuffer command_buffer, Scene& scene);
    void barrier_vr_swapchain(vk::CommandBuffer command_buffer, vk::Image swapchain_image);
    void trace(vk::CommandBuffer command_buffer, Scene& scene, size_t command_pool_id, vk::Extent2D extent);
    void copy_to_vr_swapchain(vk::CommandBuffer command_buffer, vk::Image swapchain_image, size_t command_pool_id, vk::Extent2D extent);
    void end_recording(vk::CommandBuffer command_buffer, size_t command_pool_id);

    void create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, size_t command_pool_size);
    // OpenXR doesn't expose Storage bit so we have to first render to another image and copy
    void create_descriptor_sets(vk::DescriptorPool descriptor_pool, size_t command_pool_size);
private:
    vk::Device m_device;
    VmaAllocator m_allocator;
    vk::Queue m_queue;
    Imgui_render m_imgui_render;
    Texture m_noise_texture;
    Texture m_scene_texture;
    Sampler m_sampler;
    Raytracing_pipeline m_pipeline;
    Blas m_blas;

    Vma_buffer staging;

    std::vector<vk::DescriptorSet> m_descriptor_sets;
};

}
