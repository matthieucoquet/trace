#pragma once

#include "vk_common.h"
#include "allocation.h"
#include <imgui.h>

namespace vulkan
{
class Context;

class Imgui_render
{
public:
    Imgui_render(Context& context, vk::Extent2D extent, uint32_t swapchain_size, const std::vector<vk::ImageView>& image_views);
    Imgui_render(const Imgui_render& other) = delete;
    Imgui_render(Imgui_render&& other) = delete;
    Imgui_render& operator=(const Imgui_render& other) = delete;
    Imgui_render& operator=(Imgui_render&& other) = delete;
    ~Imgui_render();

    void draw(ImDrawData* draw_data, vk::CommandBuffer command_buffer, size_t frame_id);
private:
    vk::Device m_device;
    vk::RenderPass m_render_pass;
    std::vector<vk::Framebuffer> m_framebuffers;

    vk::Sampler m_font_sampler;
    vk::DescriptorSetLayout m_descriptor_set_layout;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    vk::PipelineLayout m_pipeline_layout;
    vk::Pipeline m_pipeline;

    vulkan::Allocated_image m_font_image;
    vk::ImageView m_font_image_view;

    VmaAllocator m_allocator;

    std::vector<uint32_t> m_size_index_buffer;
    std::vector<vulkan::Allocated_buffer> m_index_buffer;
    std::vector<uint32_t> m_size_vertex_buffer;
    std::vector<vulkan::Allocated_buffer> m_vertex_buffer;

    void create_render_pass(vk::Format swapchain_format);
    void create_pipeline(Context& context, vk::Extent2D extent);
    void create_fonts_texture(Context& context);

    void setup_render_state(ImDrawData* draw_data, vk::CommandBuffer command_buffer, size_t frame_id, int fb_width, int fb_height);
};

}