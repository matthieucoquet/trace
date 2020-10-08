#pragma once
#include "vk_common.hpp"

#include "raytracing_pipeline.hpp"
#include "acceleration_structure.hpp"
#include "vma_buffer.hpp"
#include "vma_image.hpp"
#include "core/scene.hpp"

namespace vulkan
{

class Context;

struct Per_frame
{
    Tlas tlas;
    Vma_buffer objects;
    Vma_image storage_image;
    vk::ImageView image_view;
};

class Renderer
{
public:
    std::vector<Per_frame> per_frame;
    Vma_buffer material_buffer;

    Renderer(Context& context, Scene& scene);
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = default;
    Renderer& operator=(Renderer&& other) = default;
    ~Renderer();

    void update_per_frame_data(Scene& scene, size_t command_pool_id);
    void start_recording(vk::CommandBuffer command_buffer, Scene& scene, vk::Image swapchain_image, size_t command_pool_id, vk::Extent2D extent);
    void end_recording(vk::CommandBuffer command_buffer, size_t command_pool_id);

    void create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, vk::Format format, size_t command_pool_size);
    // OpenXR doesn't expose Storage bit so we have to first render to another image and copy
    void create_descriptor_sets(vk::DescriptorPool descriptor_pool, size_t command_pool_size);
private:
    vk::Device m_device;
    vk::Queue m_queue;
    Raytracing_pipeline m_pipeline;
    Blas m_blas;

    std::vector<vk::DescriptorSet> m_descriptor_sets;
};

}