#pragma once
#include "vk_common.hpp"
#include "vma_buffer.hpp"
#include "vma_image.hpp"
#include "texture.hpp"
#include <shaderc/shaderc.hpp>
#include <imgui.h>

namespace sdf_editor::vulkan
{
class Context;

class Imgui_render
{
public:
    std::vector<Texture> result_textures;
    Sampler result_sampler;

    Imgui_render(Context& context, vk::Extent2D extent, size_t command_pool_size);
    Imgui_render(const Imgui_render& other) = delete;
    Imgui_render(Imgui_render&& other) = delete;
    Imgui_render& operator=(const Imgui_render& other) = delete;
    Imgui_render& operator=(Imgui_render&& other) = delete;
    ~Imgui_render();

    void draw(ImDrawData* draw_data, vk::CommandBuffer command_buffer, size_t command_pool_id);
private:
    vk::Device m_device;
    VmaAllocator m_allocator;
    vk::RenderPass m_render_pass;
    std::vector<vk::Framebuffer> m_framebuffers;
    vk::Extent2D m_extent;

    shaderc::Compiler m_compiler;
    shaderc::CompileOptions m_group_compile_options;

    vk::Sampler m_font_sampler;
    vk::DescriptorSetLayout m_descriptor_set_layout;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    vk::PipelineLayout m_pipeline_layout;
    vk::Pipeline m_pipeline;

    Vma_image m_font_image;
    vk::ImageView m_font_image_view;

    std::vector<uint32_t> m_size_index_buffer;
    std::vector<Vma_buffer> m_index_buffer;
    std::vector<uint32_t> m_size_vertex_buffer;
    std::vector<Vma_buffer> m_vertex_buffer;

    void create_render_pass(vk::Format swapchain_format);
    void create_pipeline(Context& context);
    void create_fonts_texture(Context& context);

    void setup_render_state(ImDrawData* draw_data, vk::CommandBuffer command_buffer, size_t command_pool_id, int fb_width, int fb_height);
    [[nodiscard]] vk::ShaderModule compile_glsl_file(const char* filename, shaderc_shader_kind shader_kind) const;
};

}