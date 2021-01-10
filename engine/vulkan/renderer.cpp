#include "renderer.hpp"
#include "context.hpp"
#include "command_buffer.hpp"

#include <iostream>
#undef MemoryBarrier

namespace vulkan
{

Renderer::Renderer(Context& context, Scene& scene) :
    m_device(context.device),
    m_allocator(context.allocator),
    m_queue(context.graphics_queue),
    m_noise_texture(context, "textures/lut_noise.png"),
    m_pipeline(context, scene, m_noise_texture.sampler),
    m_blas(context)
{
    One_time_command_buffer command_buffer(context.device, context.command_pool, context.graphics_queue);
    m_blas.build(command_buffer.command_buffer, true);
    command_buffer.submit_and_wait_idle();
}

Renderer::~Renderer()
{
    for (auto& data : per_frame) {
        m_device.destroyImageView(data.image_view);
    }
}

void Renderer::start_recording(vk::CommandBuffer command_buffer, Scene& scene)
{
    command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    if (scene.shaders.pipeline_dirty) {
        /*m_queue.waitIdle();
        m_device.destroyPipeline(m_pipeline.pipeline);
        m_pipeline.create_pipeline(scene);
        auto temp_buffer_aligned = m_pipeline.create_shader_binding_table();
        Buffer_from_staged buffer_and_staged(
            m_device, m_allocator, command_buffer,
            vk::BufferCreateInfo{
                .size = temp_buffer_aligned.size(),
                .usage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress
            },
            temp_buffer_aligned.data());
        m_pipeline.shader_binding_table = std::move(buffer_and_staged.result);
        staging = std::move(buffer_and_staged.staging);
        scene.shaders.pipeline_dirty = false;

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            {}, {},
            vk::BufferMemoryBarrier{
                .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                .dstAccessMask = vk::AccessFlagBits::eShaderRead,
                .buffer = m_pipeline.shader_binding_table.buffer,
                .offset = 0u,
                .size = VK_WHOLE_SIZE
            }, {});*/
    }
}

void Renderer::barrier_vr_swapchain(vk::CommandBuffer command_buffer, vk::Image swapchain_image)
{
    //  Swapchain to dst
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = {},
            .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapchain_image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        });
}

void Renderer::trace(vk::CommandBuffer command_buffer, Scene& scene, size_t command_pool_id, vk::Extent2D extent)
{
    //per_frame[command_pool_id].tlas.update(command_buffer, scene, false);
    /*command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        {},
        vk::MemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR,
            .dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR,
        },
        {}, {});*/

    vk::DeviceAddress table_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = m_pipeline.shader_binding_table.buffer });
    vk::StridedDeviceAddressRegionKHR raygen_shader_entry{
        .deviceAddress = table_address,
        .stride = m_pipeline.shader_binding_table_stride,
        .size = m_pipeline.shader_binding_table_stride,
    };

    vk::StridedDeviceAddressRegionKHR miss_shader_entry{
        .deviceAddress = table_address + m_pipeline.offset_miss_group,
        .stride = m_pipeline.shader_binding_table_stride,
        .size = m_pipeline.shader_binding_table_stride * vk::DeviceSize(m_pipeline.nb_group_miss)
    };

    vk::StridedDeviceAddressRegionKHR hit_shader_entry{
        .deviceAddress = table_address + m_pipeline.offset_hit_group,
        .stride = m_pipeline.shader_binding_table_stride,
        .size = m_pipeline.shader_binding_table_stride * vk::DeviceSize(2 * m_pipeline.nb_group_primary)
    };

    vk::StridedDeviceAddressRegionKHR callable_shader_entry{};

    command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline.pipeline);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline.pipeline_layout, 0, m_descriptor_sets[command_pool_id], {});

    command_buffer.pushConstants(
        m_pipeline.pipeline_layout,
        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eIntersectionKHR |
        vk::ShaderStageFlagBits::eAnyHitKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, 0,
        sizeof(Scene_global), &scene.scene_global);

    command_buffer.traceRaysKHR(
        &raygen_shader_entry,
        &miss_shader_entry,
        &hit_shader_entry,
        &callable_shader_entry,
        //100, 100,
        extent.width,
        extent.height,
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
            .image = per_frame[command_pool_id].storage_image.image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        });
}

void Renderer::copy_to_vr_swapchain(vk::CommandBuffer command_buffer, vk::Image swapchain_image, size_t command_pool_id, vk::Extent2D extent)
{
    command_buffer.copyImage(
        per_frame[command_pool_id].storage_image.image, vk::ImageLayout::eTransferSrcOptimal,
        swapchain_image, vk::ImageLayout::eTransferDstOptimal,
        vk::ImageCopy{
            .srcSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            },
            .srcOffset = { 0, 0, 0 },
            .dstSubresource = {
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

void Renderer::end_recording(vk::CommandBuffer command_buffer, size_t command_pool_id)
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
            .image = per_frame[command_pool_id].storage_image.image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1u,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        });
}

void Renderer::create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, vk::Format format, size_t command_pool_size)
{
    per_frame.reserve(command_pool_size);
    //One_time_command_buffer command_buffer(m_device, context.command_pool, context.graphics_queue);
    for (size_t i = 0u; i < command_pool_size; i++)
    {
        One_time_command_buffer command_buffer(m_device, context.command_pool, context.graphics_queue);
        // Images
        Vma_image image(
            m_device, context.allocator,
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
            VMA_MEMORY_USAGE_GPU_ONLY);
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

        Vma_buffer material_buffer = Vma_buffer(
            context.device, context.allocator,
            vk::BufferCreateInfo{
                .size = sizeof(Material) * scene.materials.size(),
                .usage = vk::BufferUsageFlagBits::eStorageBuffer },
                VmaAllocationCreateInfo {
                    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
                    .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
                });
        Vma_buffer lights_buffer = Vma_buffer(
            context.device, context.allocator,
            vk::BufferCreateInfo{
                .size = sizeof(Light) * scene.lights.size(),
                .usage = vk::BufferUsageFlagBits::eStorageBuffer },
                VmaAllocationCreateInfo{
                    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
                    .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
                });
        per_frame.push_back(Per_frame{
            .tlas = {command_buffer.command_buffer, context, m_blas, scene},
            .materials = std::move(material_buffer),
            .lights = std::move(lights_buffer),
            .storage_image = std::move(image),
            .image_view = image_view
            });
        command_buffer.submit_and_wait_idle();
    }
    //command_buffer.submit_and_wait_idle();
    for (size_t i = 0u; i < command_pool_size; i++) {
        per_frame[i].materials.copy(scene.materials.data(), sizeof(Material) * scene.materials.size());
        per_frame[i].lights.copy(scene.lights.data(), sizeof(Light) * scene.lights.size());
        per_frame[i].materials.flush();
        per_frame[i].lights.flush();
    }
}

void Renderer::update_per_frame_data(Scene& /*scene*/, size_t /*command_pool_id*/)
{
    /*per_frame[command_pool_id].materials.copy(scene.materials.data(), sizeof(Material) * scene.materials.size());
    per_frame[command_pool_id].lights.copy(scene.lights.data(), sizeof(Light) * scene.lights.size());
    per_frame[command_pool_id].materials.flush();
    per_frame[command_pool_id].lights.flush();*/
}

void Renderer::create_descriptor_sets(vk::DescriptorPool descriptor_pool, size_t command_pool_size)
{
    std::vector<vk::DescriptorSetLayout> layouts(command_pool_size, m_pipeline.descriptor_set_layout);
    m_descriptor_sets = m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()});

    for (size_t i = 0; i < command_pool_size; i++)
    {
        vk::WriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{
            .accelerationStructureCount = 1u,
            .pAccelerationStructures = &(per_frame[i].tlas.acceleration_structure)
        };

        vk::DescriptorImageInfo image_info{
            .imageView = per_frame[i].image_view,
            .imageLayout = vk::ImageLayout::eGeneral
        };

        vk::DescriptorBufferInfo material_info{
            .buffer = per_frame[i].materials.buffer,
            .offset = 0u,
            .range = VK_WHOLE_SIZE
        };
        vk::DescriptorBufferInfo light_info{
            .buffer = per_frame[i].lights.buffer,
            .offset = 0u,
            .range = VK_WHOLE_SIZE
        };
        vk::DescriptorImageInfo noise_info{
            .sampler = m_noise_texture.sampler,
            .imageView = m_noise_texture.image_view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

        m_device.updateDescriptorSets(std::array{
            vk::WriteDescriptorSet{
                .pNext = &descriptor_acceleration_structure_info,
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eAccelerationStructureKHR },
            vk::WriteDescriptorSet{
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageImage,
                .pImageInfo = &image_info},
            vk::WriteDescriptorSet{
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &noise_info},
            vk::WriteDescriptorSet{
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 3,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &material_info},
            vk::WriteDescriptorSet{
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 4,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &light_info},
            }, {});
    }
}

}
