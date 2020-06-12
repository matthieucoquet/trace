#include "renderer.h"
#include "context.h"
#include "command_buffer.h"

#include <iostream>

namespace vulkan
{

Renderer::Renderer(Context& context, Scene& scene) :
    m_device(context.device),
    m_queue(context.graphics_queue),
    m_command_pool(context.command_pool),
    m_pipeline(context),
    m_blas(context, scene.aabbs_buffer.buffer, static_cast<uint32_t>(scene.aabbs.size())),
    m_tlas(context, m_blas, scene)
{}

Renderer::~Renderer()
{
    /*for (size_t i = 0; i < max_frames_in_flight; i++)
    {
        m_device.destroySemaphore(m_semaphore_available_in_flight[i]);
        m_device.destroySemaphore(m_semaphore_finished_in_flight[i]);
        m_device.destroyFence(m_fence_in_flight[i]);
        // Don't need to destroy m_images_in_flight
    }
    m_device.destroyImageView(m_image_view);
    m_device.destroyDescriptorPool(m_descriptor_pool);*/
    for (auto& command_buffer : m_command_buffers) {
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
    }
}

void Renderer::render(Scene& scene, uint32_t swapchain_index)
{
    update_uniforms(scene, swapchain_index);

    vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
    m_queue.submit(
        vk::SubmitInfo()
        .setWaitSemaphoreCount(0u)
        .setPWaitSemaphores(nullptr)
        .setPWaitDstStageMask(nullptr)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&m_command_buffers[swapchain_index])
        .setSignalSemaphoreCount(0)
        .setPSignalSemaphores(nullptr),
        {});
}

void Renderer::create_command_buffers(/*Context& context*/const std::vector<vk::Image>& images, uint32_t swapchain_size, vk::Extent2D extent)
{
    m_command_buffers = m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
        .setCommandPool(m_command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(swapchain_size));

    size_t framebuffer_id = 0u;
    for (auto& command_buffer : m_command_buffers)
    {
        command_buffer.begin(vk::CommandBufferBeginInfo());

        auto raygen_shader_entry = vk::StridedBufferRegionKHR()
            .setBuffer(m_pipeline.shader_binding_table.buffer)
            .setOffset(0u)
            .setStride(m_pipeline.raytracing_properties.shaderGroupHandleSize)
            .setSize(m_pipeline.raytracing_properties.shaderGroupHandleSize);

        auto miss_shader_entry = vk::StridedBufferRegionKHR()
            .setBuffer(m_pipeline.shader_binding_table.buffer)
            .setOffset(m_pipeline.raytracing_properties.shaderGroupHandleSize)
            .setStride(m_pipeline.raytracing_properties.shaderGroupHandleSize)
            .setSize(m_pipeline.raytracing_properties.shaderGroupHandleSize);

        auto hit_shader_entry = vk::StridedBufferRegionKHR()
            .setBuffer(m_pipeline.shader_binding_table.buffer)
            .setOffset(m_pipeline.raytracing_properties.shaderGroupHandleSize * vk::DeviceSize(2u))
            .setStride(m_pipeline.raytracing_properties.shaderGroupHandleSize)
            .setSize(m_pipeline.raytracing_properties.shaderGroupHandleSize * vk::DeviceSize(2u));

        auto callable_shader_entry = vk::StridedBufferRegionKHR();

        command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline.pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline.pipeline_layout, 0, m_descriptor_sets[framebuffer_id], {});

        //  Swapchain to dst
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            {}, {}, {},
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(images[framebuffer_id])
            .setSrcAccessMask({})
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setLevelCount(1u)
                .setLayerCount(1)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor)));

        command_buffer.traceRaysKHR(
            &raygen_shader_entry,
            &miss_shader_entry,
            &hit_shader_entry,
            &callable_shader_entry,
            extent.width,
            extent.height,
            1u);

        //  Img to source
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            vk::PipelineStageFlagBits::eTransfer,
            {}, {}, {},
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(m_storage_images[framebuffer_id].image)
            .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setLevelCount(1u)
                .setLayerCount(1)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor)));

        command_buffer.copyImage(
            m_storage_images[framebuffer_id].image, vk::ImageLayout::eTransferSrcOptimal,
            images[framebuffer_id], vk::ImageLayout::eTransferDstOptimal,
            vk::ImageCopy()
            .setSrcOffset(vk::Offset3D(0, 0, 0))
            .setSrcSubresource(vk::ImageSubresourceLayers()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setMipLevel(0u)
                .setLayerCount(1u)
                .setBaseArrayLayer(0u))
            .setDstOffset(vk::Offset3D(0, 0, 0))
            .setDstSubresource(vk::ImageSubresourceLayers()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setMipLevel(0u)
                .setLayerCount(1u)
                .setBaseArrayLayer(0u))
            .setExtent(vk::Extent3D(extent, 1u))
        );

        //  Img to storage
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            {}, {}, {},
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(m_storage_images[framebuffer_id].image)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlagBits::eShaderWrite)
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setLevelCount(1u)
                .setLayerCount(1)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor)));

        //  Swapchain to present
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eAllCommands,
            {}, {}, {},
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(images[framebuffer_id])
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask({})
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setLevelCount(1u)
                .setLayerCount(1)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor)));
        command_buffer.end();
        framebuffer_id++;
    }
}

void Renderer::create_uniforms(Context& context, uint32_t swapchain_size)
{
    m_scene_uniforms.reserve(swapchain_size);
    for (uint32_t i = 0u; i < swapchain_size; i++) {
        m_scene_uniforms.emplace_back(
             vk::BufferCreateInfo()
                 .setSize(sizeof(Scene_global))
                 .setUsage(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eRayTracingKHR),
             VMA_MEMORY_USAGE_CPU_TO_GPU,
             context.device, context.allocator);
    }

}

void Renderer::update_uniforms(Scene& scene, uint32_t swapchain_index)
{
    //std::cout << scene.time << std::endl;
    m_scene_uniforms[swapchain_index].copy(reinterpret_cast<void*>(&scene.scene_global), sizeof(Scene_global));
}

void Renderer::create_descriptor_sets(const Scene& scene, uint32_t swapchain_size)
{
    std::array pool_sizes
    {
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eAccelerationStructureKHR)
            .setDescriptorCount(swapchain_size),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eStorageImage)
            .setDescriptorCount(swapchain_size),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(swapchain_size),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(swapchain_size)
    };
    m_descriptor_pool = m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo()
        .setPoolSizeCount(static_cast<uint32_t>(pool_sizes.size()))
        .setPPoolSizes(pool_sizes.data())
        .setMaxSets(swapchain_size));

    std::vector<vk::DescriptorSetLayout> layouts(swapchain_size, m_pipeline.descriptor_set_layout);
    m_descriptor_sets = m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(m_descriptor_pool)
        .setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
        .setPSetLayouts(layouts.data()));

    for (uint32_t i = 0; i < swapchain_size; i++)
    {
        auto descriptor_acceleration_structure_info = vk::WriteDescriptorSetAccelerationStructureKHR()
            .setAccelerationStructureCount(1u)
            .setPAccelerationStructures(&m_tlas.acceleration_structure);

        auto image_info = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(m_image_views[i]);

        auto primitives_info = vk::DescriptorBufferInfo()
            .setBuffer(scene.primitives_buffer.buffer)
            .setOffset(0u)
            .setRange(VK_WHOLE_SIZE);

        auto time_uniform_info = vk::DescriptorBufferInfo()
            .setBuffer(m_scene_uniforms[i].buffer)
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

void Renderer::create_synchronization()
{
    /*for (size_t i = 0; i < max_frames_in_flight; i++)
    {
        m_semaphore_available_in_flight[i] = m_device.createSemaphore({});
        m_semaphore_finished_in_flight[i] = m_device.createSemaphore({});
        m_fence_in_flight[i] = m_device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
        m_fence_swapchain_in_flight[i] = vk::Fence();
    }*/
    //m_render_fence = m_device.createFence(vk::FenceCreateInfo());
}

void Renderer::create_storage_image(Context& context, vk::Extent2D extent, vk::Format format, uint32_t swapchain_size)
{
    m_scene_uniforms.reserve(swapchain_size);
    m_scene_uniforms.reserve(swapchain_size);
    One_time_command_buffer command_buffer(m_device, context.command_pool, context.graphics_queue);
    for (uint32_t i = 0; i < swapchain_size; i++)
    {
        // One image per swapchain image?
        m_storage_images.emplace_back(
            vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setExtent(vk::Extent3D(extent, 1))
            .setMipLevels(1u)
            .setArrayLayers(1u)
            .setFormat(format)
            .setTiling(vk::ImageTiling::eOptimal)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage),
            VMA_MEMORY_USAGE_GPU_ONLY,
            m_device, context.allocator);

        m_image_views.push_back(m_device.createImageView(vk::ImageViewCreateInfo()
            .setImage(m_storage_images.back().image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseMipLevel(0u)
                .setLevelCount(1u)
                .setBaseArrayLayer(0u)
                .setLayerCount(1u))));

        auto barrier = vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(m_storage_images.back().image)
            .setSrcAccessMask({})
            .setDstAccessMask({})
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setLevelCount(1u)
                .setLayerCount(1)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor));
        command_buffer.command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            {}, {}, {}, barrier);
    }
    command_buffer.submit_and_wait_idle();
}


void Renderer::reload_pipeline(Context& /*context*/)
{
    /*for (auto& command_buffer : m_command_buffers) {
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
    }
    m_pipeline.reload(context);
    create_command_buffers();*/
}

}