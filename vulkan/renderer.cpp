#include "renderer.hpp"
#include "context.hpp"
#include "command_buffer.hpp"

#include <iostream>

namespace vulkan
{

Renderer::Renderer(Context& context, Scene& scene) :
    m_device(context.device),
    m_queue(context.graphics_queue),
    m_command_pool(context.command_pool),
    m_pipeline(context, scene),
    m_blas(context)
{

}

Renderer::~Renderer()
{
    for (auto& data : per_frame) {
        m_device.destroyImageView(data.image_view);
    }
}

void Renderer::start_recording(vk::CommandBuffer command_buffer, Scene& scene, vk::Image swapchain_image, uint32_t swapchain_id, vk::Extent2D extent)
{
    command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    per_frame[swapchain_id].tlas.update(command_buffer, scene, false);
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        {}, {}, {}, {});

    vk::StridedBufferRegionKHR raygen_shader_center_entry{
        .buffer = m_pipeline.shader_binding_table.buffer,
        .offset = 0u,
        .stride = m_pipeline.raytracing_properties.shaderGroupHandleSize,
        .size = m_pipeline.raytracing_properties.shaderGroupHandleSize,
    };
    vk::StridedBufferRegionKHR raygen_shader_side_entry{
        .buffer = m_pipeline.shader_binding_table.buffer,
        .offset = m_pipeline.offset_raygen_side_group,
        .stride = m_pipeline.raytracing_properties.shaderGroupHandleSize,
        .size = m_pipeline.raytracing_properties.shaderGroupHandleSize,
    };

    vk::StridedBufferRegionKHR miss_shader_entry{
        .buffer = m_pipeline.shader_binding_table.buffer,
        .offset = m_pipeline.offset_miss_group,
        .stride = m_pipeline.raytracing_properties.shaderGroupHandleSize,
        .size = m_pipeline.raytracing_properties.shaderGroupHandleSize
    };

    vk::StridedBufferRegionKHR hit_shader_entry{
        .buffer = m_pipeline.shader_binding_table.buffer,
        .offset = m_pipeline.offset_hit_group,
        .stride = m_pipeline.raytracing_properties.shaderGroupHandleSize,
        .size = m_pipeline.raytracing_properties.shaderGroupHandleSize * vk::DeviceSize(2u)
    };

    vk::StridedBufferRegionKHR callable_shader_entry{};

    command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline.pipeline);

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline.pipeline_layout, 0, m_descriptor_sets[swapchain_id], {});

    //  Swapchain to dst
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = {},
            .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,  // color ?
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapchain_image,
            .subresourceRange = /*vk::ImageSubresourceRange*/{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        });

    constexpr unsigned int foveated_rate = 8u;
    extent.width *= 2;
    assert(extent.width % foveated_rate == 0);
    assert(extent.height % 4 == 0);


    command_buffer.traceRaysKHR(
        &raygen_shader_side_entry,
        &miss_shader_entry,
        &hit_shader_entry,
        &callable_shader_entry,
        extent.width / foveated_rate,
        extent.height / foveated_rate,
        1u);

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        {}, {}, {}, {});

    command_buffer.traceRaysKHR(
        &raygen_shader_center_entry,
        &miss_shader_entry,
        &hit_shader_entry,
        &callable_shader_entry,
        extent.width / 2,
        extent.height / 2,
        1u);

    //  Img to source
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        vk::PipelineStageFlagBits::eTransfer,
        {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
            .dstAccessMask = vk::AccessFlagBits::eTransferRead,
            .oldLayout = vk::ImageLayout::eGeneral,
            .newLayout = vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = per_frame[swapchain_id].storage_image.image,
            .subresourceRange = /*vk::ImageSubresourceRange*/ {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        });

    command_buffer.copyImage(
        per_frame[swapchain_id].storage_image.image, vk::ImageLayout::eTransferSrcOptimal,
        swapchain_image, vk::ImageLayout::eTransferDstOptimal,
        vk::ImageCopy{
            .srcSubresource = /*vk::ImageSubresourceLayers*/{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            },
            .srcOffset = { 0, 0, 0 },
            .dstSubresource = /*vk::ImageSubresourceLayers*/{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            },
            .dstOffset = { 0, 0, 0 },
            .extent = { extent.width, extent.height, 1u }
        }
    );
}

void Renderer::end_recording(vk::CommandBuffer command_buffer, vk::Image swapchain_image, uint32_t swapchain_id)
{
    //  Img to storage
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferRead,
            .dstAccessMask = vk::AccessFlagBits::eShaderWrite,
            .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
            .newLayout = vk::ImageLayout::eGeneral,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = per_frame[swapchain_id].storage_image.image,
            .subresourceRange = /*vk::ImageSubresourceRange*/{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        });

    //  Swapchain to attachment optiomal
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        //vk::PipelineStageFlagBits::eAllCommands,
        {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapchain_image,
            .subresourceRange = /*vk::ImageSubresourceRange*/{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        });
}

void Renderer::create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, vk::Format format, uint32_t swapchain_size)
{
    per_frame.reserve(swapchain_size);
    One_time_command_buffer command_buffer(m_device, context.command_pool, context.graphics_queue);
    for (uint32_t i = 0u; i < swapchain_size; i++) {
        // Images
        auto image = Vma_image(
            vk::ImageCreateInfo{
                .imageType = vk::ImageType::e2D,
                .format = format,
                .extent = {extent.width, extent.height, 1},
                .mipLevels = 1u,
                .arrayLayers = 1u,
                .samples = vk::SampleCountFlagBits::e1,
                .tiling = vk::ImageTiling::eOptimal,
                .usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage,
                .sharingMode = vk::SharingMode::eExclusive,
                .initialLayout = vk::ImageLayout::eUndefined
            },
            VMA_MEMORY_USAGE_GPU_ONLY,
            m_device, context.allocator);
        auto image_view = m_device.createImageView(
            vk::ImageViewCreateInfo{
               .image = image.image,
               .viewType = vk::ImageViewType::e2D,
               .format = format,
               .subresourceRange = {
                   .aspectMask = vk::ImageAspectFlagBits::eColor,
                   .baseMipLevel = 0u,
                   .levelCount = 1u,
                   .baseArrayLayer = 0u,
                   .layerCount = 1u} } );
        command_buffer.command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            {}, {}, {},
            vk::ImageMemoryBarrier{
                .srcAccessMask = {},
                .dstAccessMask = {},
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eGeneral,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = image.image,
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1u,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }});

        Vma_buffer scene_buffer = Vma_buffer(
            vk::BufferCreateInfo{
                    .size = sizeof(Scene_global),
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eRayTracingKHR },
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            context.device, context.allocator);
        Vma_buffer prim_buffer = Vma_buffer(
            vk::BufferCreateInfo{
                    .size = sizeof(Primitive) * scene.primitives.size(),
                    .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eRayTracingKHR },
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            context.device, context.allocator);
        per_frame.push_back(Per_frame{
            .tlas = Tlas(command_buffer.command_buffer, context, m_blas, scene),
            .scene_uniform = std::move(scene_buffer),
            .primitives = std::move(prim_buffer),
            .storage_image = std::move(image),
            .image_view = image_view
            });
    }
    command_buffer.submit_and_wait_idle();
}

void Renderer::update_per_frame_data(Scene& scene, uint32_t swapchain_index)
{
    per_frame[swapchain_index].scene_uniform.copy(reinterpret_cast<void*>(&scene.scene_global), sizeof(Scene_global));
    per_frame[swapchain_index].primitives.copy(scene.primitives.data(), sizeof(Primitive) * scene.primitives.size());
}

void Renderer::create_descriptor_sets(vk::DescriptorPool descriptor_pool, uint32_t swapchain_size)
{
    std::vector<vk::DescriptorSetLayout> layouts(swapchain_size, m_pipeline.descriptor_set_layout);
    m_descriptor_sets = m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
        .setPSetLayouts(layouts.data()));

    for (uint32_t i = 0; i < swapchain_size; i++)
    {
        auto descriptor_acceleration_structure_info = vk::WriteDescriptorSetAccelerationStructureKHR()
            .setAccelerationStructureCount(1u)
            .setPAccelerationStructures(&(per_frame[i].tlas.acceleration_structure));

        auto image_info = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(per_frame[i].image_view);

        auto primitives_info = vk::DescriptorBufferInfo()
            .setBuffer(per_frame[i].primitives.buffer)
            .setOffset(0u)
            .setRange(VK_WHOLE_SIZE);

        auto time_uniform_info = vk::DescriptorBufferInfo()
            .setBuffer(per_frame[i].scene_uniform.buffer)
            .setOffset(0u)
            .setRange(VK_WHOLE_SIZE);

        m_device.updateDescriptorSets(std::array{
            vk::WriteDescriptorSet()
                .setPNext(&descriptor_acceleration_structure_info)
                .setDstSet(m_descriptor_sets[i])
                .setDstBinding(0)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
                .setDescriptorCount(1),
            vk::WriteDescriptorSet()
                .setDstSet(m_descriptor_sets[i])
                .setDstBinding(1)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setDescriptorCount(1)
                .setPImageInfo(&image_info),
            vk::WriteDescriptorSet()
                .setDstSet(m_descriptor_sets[i])
                .setDstBinding(2)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setDescriptorCount(1)
                .setPBufferInfo(&primitives_info),
            vk::WriteDescriptorSet()
                .setDstSet(m_descriptor_sets[i])
                .setDstBinding(3)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDescriptorCount(1)
                .setPBufferInfo(&time_uniform_info)
            }, {});
    }
}

void Renderer::reload_pipeline(Context& context)
{
    m_pipeline.reload(context);
}

}