#include "imgui_render.hpp"
#include "context.hpp"
#include "command_buffer.hpp"
#include <imgui.h>
#include <fmt/core.h>
#include <filesystem>
#include <fstream>

namespace vulkan
{

Imgui_render::Imgui_render(Context& context, vk::Extent2D extent, uint32_t command_pool_size, const std::vector<vk::ImageView>& image_views):
    m_device(context.device),
    m_allocator(context.allocator),
    m_extent(extent)
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_vulkan_hpp";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.DisplaySize = ImVec2((float)extent.width, (float)extent.height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    create_render_pass(vk::Format::eR8G8B8A8Unorm);
    create_pipeline(context);
    create_fonts_texture(context);
    m_size_index_buffer.resize(command_pool_size, 0u);
    m_index_buffer.resize(command_pool_size);
    m_size_vertex_buffer.resize(command_pool_size, 0u);
    m_vertex_buffer.resize(command_pool_size);

    m_framebuffers.reserve(image_views.size());
    for (vk::ImageView image_view : image_views)
    {
        m_framebuffers.push_back(m_device.createFramebuffer(vk::FramebufferCreateInfo{
            .renderPass = m_render_pass,
            .attachmentCount = 1u,
            .pAttachments = &image_view,
            .width = extent.width,
            .height = extent.height,
            .layers = 1 }));
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
    vk::AttachmentDescription attachment{
        .format = swapchain_format,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eColorAttachmentOptimal };

    vk::AttachmentReference color_attachment_ref{
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal };

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref };

    // I don't think i need this with openxr swapchain...
    vk::SubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite };

    m_render_pass = m_device.createRenderPass(vk::RenderPassCreateInfo{
        .attachmentCount = 1u,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency });
}

void Imgui_render::create_pipeline(Context& context)
{
    vk::ShaderModule vertex_module = compile_glsl_file("imgui_vert_shader.glsl", shaderc_vertex_shader);
    vk::ShaderModule fragment_module = compile_glsl_file("imgui_frag_shader.glsl", shaderc_fragment_shader);

    m_font_sampler = m_device.createSampler(vk::SamplerCreateInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .maxAnisotropy = 1.0f,
        .minLod = 0.0f,
        .maxLod = 1.0f,
        });

    constexpr uint32_t binding = 0u;
    vk::DescriptorSetLayoutBinding layout_binding{
        .binding = binding,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1u,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = &m_font_sampler };
    m_descriptor_set_layout = m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
        .bindingCount = 1u,
        .pBindings = &layout_binding });
    m_descriptor_sets = m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
        .descriptorPool = context.descriptor_pool,
        .descriptorSetCount = 1u,
        .pSetLayouts = &m_descriptor_set_layout });

    // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
    vk::PushConstantRange push_constants{
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(float) * 4 };
    m_pipeline_layout = m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo{
        .setLayoutCount = 1u,
        .pSetLayouts = &m_descriptor_set_layout,
        .pushConstantRangeCount = 1u,
        .pPushConstantRanges = &push_constants });

    vk::PipelineShaderStageCreateInfo vertex_shader_stage_info{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = vertex_module,
        .pName = "main" };
    vk::PipelineShaderStageCreateInfo fragment_shader_stage_info{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = fragment_module,
        .pName = "main" };
    vk::PipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_info, fragment_shader_stage_info };

    vk::VertexInputBindingDescription binding_descr{
        .binding = 0,
        .stride = sizeof(ImDrawVert),
        .inputRate = vk::VertexInputRate::eVertex };
    auto attribute_descr = std::array{
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = binding,
            .format = vk::Format::eR32G32Sfloat,
            .offset = IM_OFFSETOF(ImDrawVert, pos)},
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = binding,
            .format = vk::Format::eR32G32Sfloat,
            .offset = IM_OFFSETOF(ImDrawVert, uv)},
        vk::VertexInputAttributeDescription{
            .location = 2,
            .binding = binding,
            .format = vk::Format::eR8G8B8A8Unorm,
            .offset = IM_OFFSETOF(ImDrawVert, col)}
    };
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_descr,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descr.size()),
        .pVertexAttributeDescriptions = attribute_descr.data() };
    vk::PipelineInputAssemblyStateCreateInfo input_assembly {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false };

    vk::PipelineViewportStateCreateInfo viewport_info{
        .viewportCount = 1u,
        .scissorCount = 1u };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = false,
        .lineWidth = 1.0f };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = false };

    vk::PipelineColorBlendAttachmentState color_blend_attachment{
        .blendEnable = true,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineDepthStencilStateCreateInfo depth_test{
        .depthTestEnable = false,
        .depthWriteEnable = false,
        .depthBoundsTestEnable = false,
        .stencilTestEnable = false };

    vk::PipelineColorBlendStateCreateInfo color_blending{
        .logicOpEnable = false,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment };

    std::array dynamic_states{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamic_state{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data() };

    m_pipeline = m_device.createGraphicsPipeline(nullptr, vk::GraphicsPipelineCreateInfo{
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_info,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_test,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = m_pipeline_layout,
        .renderPass = m_render_pass,
        .subpass = 0 }).value;

    m_device.destroyShaderModule(vertex_module);
    m_device.destroyShaderModule(fragment_module);
}

void Imgui_render::draw(ImDrawData* draw_data, vk::CommandBuffer command_buffer, size_t command_pool_id, size_t frame_id)
{
    auto clear_value = vk::ClearValue().setColor(std::array<float,4>{ 0.03f, 0.03f, 0.03f, 1.0f });
    command_buffer.beginRenderPass(
        vk::RenderPassBeginInfo{
            .renderPass = m_render_pass,
            .framebuffer = m_framebuffers[frame_id],
            .renderArea = { .offset = {}, .extent = m_extent },
            .clearValueCount = 1u,
            .pClearValues = &clear_value 
        },
        vk::SubpassContents::eInline);

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    auto fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    auto fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    if (draw_data->TotalVtxCount > 0)
    {
        size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        if (m_size_vertex_buffer[command_pool_id] < vertex_size)
        {
            m_size_vertex_buffer[command_pool_id] = static_cast<uint32_t>(vertex_size);
            m_vertex_buffer[command_pool_id] = Vma_buffer(
                m_device, m_allocator,
                vk::BufferCreateInfo{
                    .size = vertex_size,
                    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                    .sharingMode = vk::SharingMode::eExclusive },
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
        size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        if (m_size_index_buffer[command_pool_id] < index_size)
        {
            m_size_index_buffer[command_pool_id] = static_cast<uint32_t>(index_size);
            m_index_buffer[command_pool_id] = Vma_buffer(
                m_device, m_allocator,
                vk::BufferCreateInfo{
                    .size = index_size,
                    .usage = vk::BufferUsageFlagBits::eIndexBuffer,
                    .sharingMode = vk::SharingMode::eExclusive },
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        ImDrawIdx* index_mapped = static_cast<ImDrawIdx*>(m_index_buffer[command_pool_id].map());
        ImDrawVert* vertex_mapped = static_cast<ImDrawVert*>(m_vertex_buffer[command_pool_id].map());
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vertex_mapped, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(index_mapped, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vertex_mapped += cmd_list->VtxBuffer.Size;
            index_mapped += cmd_list->IdxBuffer.Size;
        }
        m_index_buffer[command_pool_id].unmap();
        m_vertex_buffer[command_pool_id].unmap();
    }

    setup_render_state(draw_data, command_buffer, command_pool_id, fb_width, fb_height);

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
                    setup_render_state(draw_data, command_buffer, command_pool_id, fb_width, fb_height);
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
                    command_buffer.setScissor(0, vk::Rect2D{
                        .offset = { static_cast<int32_t>(clip_rect.x), static_cast<int32_t>(clip_rect.y) },
                        .extent = { static_cast<uint32_t>(clip_rect.z - clip_rect.x), static_cast<uint32_t>(clip_rect.w - clip_rect.y) }
                    });

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

    One_time_command_buffer command_buffer(context.device, context.command_pool, context.graphics_queue);
    Image_from_staged image_and_staged(
        m_device, context.allocator, command_buffer.command_buffer,
        vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
            .mipLevels = 1u,
            .arrayLayers = 1u,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined
        },
        pixels, upload_size);
    m_font_image = std::move(image_and_staged.result);
    command_buffer.submit_and_wait_idle();

    m_font_image_view = m_device.createImageView(vk::ImageViewCreateInfo{
        .image = m_font_image.image,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Unorm,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0u,
            .levelCount = 1u,
            .baseArrayLayer = 0u,
            .layerCount = 1u } });

    vk::DescriptorImageInfo image_info{
        .sampler = m_font_sampler,
        .imageView = m_font_image_view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    m_device.updateDescriptorSets(std::array{
            vk::WriteDescriptorSet{
                .dstSet = m_descriptor_sets[0],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &image_info} }, {}),

    io.Fonts->TexID = (ImTextureID)(intptr_t)VkImage(m_font_image.image);
}

void Imgui_render::setup_render_state(ImDrawData* draw_data, vk::CommandBuffer command_buffer, size_t command_pool_id, int fb_width, int fb_height)
{
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, m_descriptor_sets, {});

    // Bind Vertex And Index Buffer:
    if (draw_data->TotalVtxCount > 0)
    {
        command_buffer.bindVertexBuffers(0u, m_vertex_buffer[command_pool_id].buffer, uint64_t(0u));
        command_buffer.bindIndexBuffer(m_index_buffer[command_pool_id].buffer, 0u, sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
    }

    command_buffer.setViewport(0, vk::Viewport{
        .x = 0, .y = 0,
        .width = static_cast<float>(fb_width),
        .height = static_cast<float>(fb_height),
        .minDepth = 0.0f,
        .maxDepth = 0.0f });

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

vk::ShaderModule Imgui_render::compile_glsl_file(const char* filename, shaderc_shader_kind shader_kind) const
{
    std::filesystem::path base_dir(SHADER_SOURCE);
    std::filesystem::path file = base_dir / filename;

    std::ifstream content(file, std::ios::ate);
    if (!content.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t file_size = (size_t)content.tellg();
    std::vector<char> buffer(file_size);
    content.seekg(0);
    content.read(buffer.data(), file_size);
    content.close();

    auto compile_result = m_compiler.CompileGlslToSpv(buffer.data(), buffer.size(), shader_kind, filename, m_group_compile_options);
    if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        fmt::print("{}\n", compile_result.GetErrorMessage());
        throw std::runtime_error("Imgui shader compilation error");
    }
    return  m_device.createShaderModule(vk::ShaderModuleCreateInfo{
        .codeSize = sizeof(shaderc::SpvCompilationResult::element_type) * std::distance(compile_result.begin(), compile_result.end()),
        .pCode = compile_result.begin() });
}

}