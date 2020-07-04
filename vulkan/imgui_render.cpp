#include "imgui_render.h"
#include "context.h"
#include <imgui.h>
#include <iostream>

namespace vulkan
{

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t __glsl_shader_vert_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
    0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
    0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
    0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
    0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
    0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
    0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
    0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
    0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
    0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
    0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
    0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
    0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
    0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
    0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
    0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
    0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
    0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
    0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
    0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
    0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
    0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
    0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
    0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
    0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
    0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
    0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
    0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
    0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
    0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
    0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
    0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
    0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
    0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
    0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
    0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
    0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t __glsl_shader_frag_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
    0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
    0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
    0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
    0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
    0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
    0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
    0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
    0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
    0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
    0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
    0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
    0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
    0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
    0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
    0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
    0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
    0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
    0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
    0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
    0x00010038
};

Imgui_render::Imgui_render(Context& context, vk::Extent2D extent, uint32_t swapchain_size, const std::vector<vk::ImageView>& image_views):
    m_device(context.device),
    m_allocator(context.allocator)
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_vulkan_hpp";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.DisplaySize = ImVec2((float)extent.width, (float)extent.height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    create_render_pass(vk::Format::eR8G8B8A8Unorm);
    create_pipeline(context, extent);
    create_fonts_texture(context);
    m_size_index_buffer.resize(swapchain_size, 0u);
    m_index_buffer.resize(swapchain_size);
    m_size_vertex_buffer.resize(swapchain_size, 0u);
    m_vertex_buffer.resize(swapchain_size);

    m_framebuffers.reserve(swapchain_size);
    for (vk::ImageView image_view : image_views)
    {
        m_framebuffers.push_back(m_device.createFramebuffer(vk::FramebufferCreateInfo()
            .setRenderPass(m_render_pass)
            .setAttachmentCount(1u)
            .setPAttachments(&image_view)
            .setWidth(extent.width)
            .setHeight(extent.height)
            .setLayers(1)));
    }
}

Imgui_render::~Imgui_render()
{
    for (auto framebuffer : m_framebuffers) {
        m_device.destroyFramebuffer(framebuffer);
    }
    m_device.destroyImageView(m_font_image_view);
    m_device.destroySampler(m_font_sampler);
    m_device.destroyDescriptorSetLayout(m_descriptor_set_layout);
    m_device.destroyPipelineLayout(m_pipeline_layout);
    m_device.destroyPipeline(m_pipeline);
    m_device.destroyRenderPass(m_render_pass);
}

void Imgui_render::create_render_pass(vk::Format swapchain_format)
{
    auto attachment = vk::AttachmentDescription()
        .setFormat(swapchain_format)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

    auto color_attachment_ref = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    auto subpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&color_attachment_ref);

    // I don't think i need this with openxr swapchain...
    auto dependency = vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    m_render_pass = m_device.createRenderPass(vk::RenderPassCreateInfo()
        .setAttachmentCount(1u)
        .setPAttachments(&attachment)
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setPDependencies(&dependency));
}

void Imgui_render::create_pipeline(Context& context, vk::Extent2D extent)
{
    vk::ShaderModule vertex_module = m_device.createShaderModule(vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(__glsl_shader_vert_spv))
        .setPCode(reinterpret_cast<const uint32_t*>(__glsl_shader_vert_spv)));

    vk::ShaderModule fragment_module = m_device.createShaderModule(vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(__glsl_shader_frag_spv))
        .setPCode(reinterpret_cast<const uint32_t*>(__glsl_shader_frag_spv)));

    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.minLod = -1000;
    info.maxLod = 1000;
    info.maxAnisotropy = 1.0f;
    m_font_sampler = m_device.createSampler(vk::SamplerCreateInfo()
        .setMinFilter(vk::Filter::eLinear)
        .setMagFilter(vk::Filter::eLinear)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setMinLod(-1000.0f)
        .setMaxLod(1000.0f)
        .setMaxAnisotropy(1.0f)
    );

    constexpr uint32_t binding = 0u;
    auto layout_binding = vk::DescriptorSetLayoutBinding()
        .setBinding(binding)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setDescriptorCount(1u)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(&m_font_sampler);
    m_descriptor_set_layout = m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(1u)
        .setPBindings(&layout_binding));
    m_descriptor_sets = m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(context.descriptor_pool)
        .setDescriptorSetCount(1u)
        .setPSetLayouts(&m_descriptor_set_layout));

    // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
    auto push_constants = vk::PushConstantRange()
        .setStageFlags(vk::ShaderStageFlagBits::eVertex)
        .setOffset(sizeof(float) * 0)
        .setSize(sizeof(float) * 4);
    m_pipeline_layout = m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo()
        .setPSetLayouts(&m_descriptor_set_layout)
        .setSetLayoutCount(1u)
        .setPushConstantRangeCount(1u)
        .setPPushConstantRanges(&push_constants));

    auto vertex_shader_stage_info = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(vertex_module)
        .setPName("main");
    auto fragment_shader_stage_info = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(fragment_module)
        .setPName("main");
    vk::PipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_info, fragment_shader_stage_info };

    auto binding_descr = vk::VertexInputBindingDescription().setBinding(0)
        .setStride(sizeof(ImDrawVert))
        .setInputRate(vk::VertexInputRate::eVertex);
    auto attribute_descr = std::array<vk::VertexInputAttributeDescription, 3>{
        vk::VertexInputAttributeDescription()
            .setBinding(binding)
            .setLocation(0)
            .setFormat(vk::Format::eR32G32Sfloat)
            .setOffset(IM_OFFSETOF(ImDrawVert, pos)),
            vk::VertexInputAttributeDescription()
            .setBinding(binding)
            .setLocation(1)
            .setFormat(vk::Format::eR32G32Sfloat)
            .setOffset(IM_OFFSETOF(ImDrawVert, uv)),
            vk::VertexInputAttributeDescription()
            .setBinding(binding)
            .setLocation(2)
            .setFormat(vk::Format::eR8G8B8Unorm)
            .setOffset(IM_OFFSETOF(ImDrawVert, col))
    };
    auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo()
        .setVertexBindingDescriptionCount(1)
        .setPVertexBindingDescriptions(&binding_descr)
        .setVertexAttributeDescriptionCount(static_cast<uint32_t>(attribute_descr.size()))
        .setPVertexAttributeDescriptions(attribute_descr.data());
    auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(false);

    auto viewport = vk::Viewport()
        .setWidth(static_cast<float>(extent.width))
        .setHeight(static_cast<float>(extent.height))
        .setMaxDepth(1.0f);
    auto scissor = vk::Rect2D().setExtent(extent);
    auto viewport_info = vk::PipelineViewportStateCreateInfo()
        .setViewportCount(1u)
        .setPViewports(&viewport)
        .setScissorCount(1u)
        .setPScissors(&scissor);

    auto rasterizer = vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setDepthBiasEnable(false);

    auto multisampling = vk::PipelineMultisampleStateCreateInfo()
        .setSampleShadingEnable(false)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    // Check blend ?
    auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(false);
    /*VkPipelineColorBlendAttachmentState color_attachment[1] = {};
    color_attachment[0].blendEnable = VK_TRUE;
    color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
    color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
    color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;*/

    auto depth_test = vk::PipelineDepthStencilStateCreateInfo()
        .setDepthTestEnable(false)
        .setDepthWriteEnable(false)
        .setDepthBoundsTestEnable(false)
        .setStencilTestEnable(false);

    auto color_blending = vk::PipelineColorBlendStateCreateInfo()
        .setLogicOpEnable(false)
        .setAttachmentCount(1)
        .setPAttachments(&color_blend_attachment);

    std::array dynamic_states{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    auto dynamic_state = vk::PipelineDynamicStateCreateInfo()
        .setDynamicStateCount(static_cast<uint32_t>(dynamic_states.size()))
        .setPDynamicStates(dynamic_states.data());

    m_pipeline = m_device.createGraphicsPipeline(nullptr, vk::GraphicsPipelineCreateInfo()
        .setStageCount(2)
        .setPStages(shader_stages)
        .setPVertexInputState(&vertex_input_info)
        .setPInputAssemblyState(&input_assembly)
        .setPViewportState(&viewport_info)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPColorBlendState(&color_blending)
        .setPDepthStencilState(&depth_test)
        .setPDynamicState(&dynamic_state)
        .setLayout(m_pipeline_layout)
        .setRenderPass(m_render_pass)
        .setSubpass(0));

    m_device.destroyShaderModule(vertex_module);
    m_device.destroyShaderModule(fragment_module);
}

void Imgui_render::draw(ImDrawData* draw_data, vk::CommandBuffer command_buffer, size_t frame_id)
{
    auto clear_value = vk::ClearValue().setColor(std::array<float,4>{ 0.03f, 0.03f, 0.03f, 1.0f });
    auto renderpass_info = vk::RenderPassBeginInfo()
        .setRenderPass(m_render_pass)
        .setFramebuffer(m_framebuffers[frame_id])
        .setRenderArea(vk::Rect2D({}, {500, 500}))
        .setClearValueCount(1u)
        .setPClearValues(&clear_value);
    command_buffer.beginRenderPass(renderpass_info, vk::SubpassContents::eInline);

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    auto fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    auto fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    if (draw_data->TotalVtxCount > 0)
    {
        size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        if (m_size_vertex_buffer[frame_id] < vertex_size)
        {
            m_size_vertex_buffer[frame_id] = static_cast<uint32_t>(vertex_size);
            m_vertex_buffer[frame_id] = Allocated_buffer(vk::BufferCreateInfo()
                .setSize(vertex_size)
                .setSharingMode(vk::SharingMode::eExclusive)
                .setUsage(vk::BufferUsageFlagBits::eVertexBuffer),
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                m_device, m_allocator);
        }
        size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        if (m_size_index_buffer[frame_id] < index_size)
        {
            m_size_index_buffer[frame_id] = static_cast<uint32_t>(index_size);
            m_index_buffer[frame_id] = Allocated_buffer(vk::BufferCreateInfo()
                .setSize(index_size)
                .setSharingMode(vk::SharingMode::eExclusive)
                .setUsage(vk::BufferUsageFlagBits::eIndexBuffer),
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                m_device, m_allocator);
        }

        ImDrawVert* index_mapped = static_cast<ImDrawVert*>(m_index_buffer[frame_id].map());
        ImDrawIdx* vertex_mapped = static_cast<ImDrawIdx*>(m_vertex_buffer[frame_id].map());
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vertex_mapped, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(index_mapped, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vertex_mapped += cmd_list->VtxBuffer.Size;
            index_mapped += cmd_list->IdxBuffer.Size;
        }
        /*VkMappedMemoryRange range[2] = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = rb->VertexBufferMemory;
        range[0].size = VK_WHOLE_SIZE;
        range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[1].memory = rb->IndexBufferMemory;
        range[1].size = VK_WHOLE_SIZE;
        err = vkFlushMappedMemoryRanges(v->Device, 2, range);*/
        m_index_buffer[frame_id].unmap();
        m_vertex_buffer[frame_id].unmap();
    }

    setup_render_state(draw_data, command_buffer, frame_id, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    setup_render_state(draw_data, command_buffer, frame_id, fb_width, fb_height);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if (clip_rect.x < 0.0f)
                        clip_rect.x = 0.0f;
                    if (clip_rect.y < 0.0f)
                        clip_rect.y = 0.0f;

                    // Apply scissor/clipping rectangle
                    command_buffer.setScissor(0, vk::Rect2D()
                        .setOffset(vk::Offset2D(static_cast<uint32_t>(clip_rect.x), static_cast<uint32_t>(clip_rect.y)))
                        .setExtent(vk::Extent2D(static_cast<uint32_t>(clip_rect.z - clip_rect.x), static_cast<uint32_t>(clip_rect.w - clip_rect.y))));

                    command_buffer.drawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
                }
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
    command_buffer.endRenderPass();
}

void Imgui_render::create_fonts_texture(Context& context)
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    size_t upload_size = width * height * 4 * sizeof(char);

    m_font_image = vulkan::Allocated_image(vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setExtent(vk::Extent3D(width, height, 1))
        .setMipLevels(1u)
        .setArrayLayers(1u)
        .setFormat(vk::Format::eR8G8B8A8Unorm)
        .setTiling(vk::ImageTiling::eOptimal)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst),
        pixels, upload_size,
        m_device, context.allocator, context.command_pool, context.graphics_queue);

    m_font_image_view = m_device.createImageView(vk::ImageViewCreateInfo()
        .setImage(m_font_image.image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(vk::Format::eR8G8B8A8Unorm)
        .setSubresourceRange(vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0u)
            .setLevelCount(1u)
            .setBaseArrayLayer(0u)
            .setLayerCount(1u)));

    auto image_info = vk::DescriptorImageInfo()
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSampler(m_font_sampler)
        .setImageView(m_font_image_view);
    m_device.updateDescriptorSets(std::array{
            vk::WriteDescriptorSet()
                .setDstSet(m_descriptor_sets[0])
                .setDstBinding(0)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eSampledImage)
                .setDescriptorCount(1)
                .setPImageInfo(&image_info) }, {}),

    io.Fonts->TexID = (ImTextureID)(intptr_t)VkImage(m_font_image.image);
}

void Imgui_render::setup_render_state(ImDrawData* draw_data, vk::CommandBuffer command_buffer, size_t frame_id, int fb_width, int fb_height)
{
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, m_descriptor_sets, {});

    // Bind Vertex And Index Buffer:
    if (draw_data->TotalVtxCount > 0)
    {
        command_buffer.bindVertexBuffers(0u, m_vertex_buffer[frame_id].buffer, uint64_t(0u));
        command_buffer.bindIndexBuffer(m_index_buffer[frame_id].buffer, 0u, sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
    }

    command_buffer.setViewport(0, vk::Viewport().setX(0).setY(0)
        .setWidth(static_cast<float>(fb_width))
        .setHeight(static_cast<float>(fb_height))
        .setMinDepth(0.0f).setMaxDepth(0.0f));

    std::array<float, 2> scale{
        2.0f / draw_data->DisplaySize.x,
        2.0f / draw_data->DisplaySize.y
    };
    std::array<float, 2> translate{
        -1.0f - draw_data->DisplayPos.x * scale[0],
        -1.0f - draw_data->DisplayPos.y * scale[1]
    };
    command_buffer.pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 0, sizeof(float) * 2, scale.data());
    command_buffer.pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 2, sizeof(float) * 2, translate.data());
}

}