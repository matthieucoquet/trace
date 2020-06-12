/*
#include "renderer.h"
#include "context.h"
#include "command_buffer.h"

namespace vulkan
{

Renderer::Renderer(Context& context, Scene& scene) :
    m_device(context.device),
    m_queue(context.graphics_queue),
    m_command_pool(context.command_pool),
    m_swapchain(context),  // TODO: Make sure swapchain image have correct usage. transfer_dest or other ??
    m_pipeline(context),
    m_blas(context, scene.aabbs_buffer.buffer, static_cast<uint32_t>(scene.aabbs.size())),
    m_tlas(context, m_blas, scene)
{
    create_uniforms(context);
    create_storage_image(context);
    create_descriptor_sets(scene);
    create_command_buffers();
    create_synchronization();
}

Renderer::~Renderer()
{
    for (size_t i = 0; i < max_frames_in_flight; i++)
    {
        m_device.destroySemaphore(m_semaphore_available_in_flight[i]);
        m_device.destroySemaphore(m_semaphore_finished_in_flight[i]);
        m_device.destroyFence(m_fence_in_flight[i]);
        // Don't need to destroy m_images_in_flight
    }
    m_device.destroyImageView(m_image_view);
    m_device.destroyDescriptorPool(m_descriptor_pool);
    for (auto& command_buffer : m_command_buffers) {
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
    }
}

void Renderer::step(Scene& scene)
{
    m_device.waitForFences(m_fence_in_flight[m_current_frame], true, std::numeric_limits<uint64_t>::max());
    auto acquire_result = m_device.acquireNextImageKHR(m_swapchain.swapchain, std::numeric_limits<uint64_t>::max(), m_semaphore_available_in_flight[m_current_frame], {});
    if (acquire_result.result == vk::Result::eErrorOutOfDateKHR) {
        assert(false);
    }
    uint32_t image_id = acquire_result.value;

    if (m_fence_swapchain_in_flight[image_id])
    {
        m_device.waitForFences(m_fence_swapchain_in_flight[image_id], true, std::numeric_limits<uint64_t>::max());
    }
    m_fence_swapchain_in_flight[image_id] = m_fence_in_flight[m_current_frame];

    update_uniforms(image_id, scene);

    vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
    m_device.resetFences(m_fence_in_flight[m_current_frame]);
    m_queue.submit(
        vk::SubmitInfo()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&m_semaphore_available_in_flight[m_current_frame])
        .setPWaitDstStageMask(&wait_stages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&m_command_buffers[image_id])
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&m_semaphore_finished_in_flight[m_current_frame]),
        m_fence_in_flight[m_current_frame]);

    auto present_result = m_queue.presentKHR(vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&m_semaphore_finished_in_flight[m_current_frame])
        .setSwapchainCount(1)
        .setPSwapchains(&m_swapchain.swapchain)
        .setPImageIndices(&image_id));
    if (present_result == vk::Result::eErrorOutOfDateKHR) {
        assert(false);
    }
    m_current_frame = (m_current_frame + 1u) % max_frames_in_flight;
}

void Renderer::create_command_buffers()
{
    auto command_buffers = m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
        .setCommandPool(m_command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(Swapchain::image_count));
    for (size_t i = 0u; i < Swapchain::image_count; i++) {
        m_command_buffers[i] = command_buffers[i];
    }

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
            .setImage(m_swapchain.images[framebuffer_id])
            .setSrcAccessMask({})
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite) //  ?
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
            m_swapchain.extent.width,
            m_swapchain.extent.height,
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
            .setImage(m_storage_image.image)
            .setSrcAccessMask({})
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead) // vk::AccessFlagBits::eColorAttachmentWrite ?
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setLevelCount(1u)
                .setLayerCount(1)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor)));

        command_buffer.copyImage(
            m_storage_image.image, vk::ImageLayout::eTransferSrcOptimal,
            m_swapchain.images[framebuffer_id], vk::ImageLayout::eTransferDstOptimal,
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
            .setExtent(vk::Extent3D(m_swapchain.extent, 1u))
        );

        //  Swapchain to present
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,  // All command?
            {}, {}, {},
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(m_swapchain.images[framebuffer_id])
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask({}) // vk::AccessFlagBits::eColorAttachmentWrite ?
            .setSubresourceRange(vk::ImageSubresourceRange()
                .setLevelCount(1u)
                .setLayerCount(1)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor)));

        //  Img to storage
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,  // raytracing // top
            {}, {}, {},
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(m_storage_image.image)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask({}) // vk::AccessFlagBits::eColorAttachmentWrite ?
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

void Renderer::create_uniforms(Context& context)
{
    for (size_t i = 0u; i < Swapchain::image_count; i++) {
         m_time_uniforms[i] = Allocated_buffer(
             vk::BufferCreateInfo()
                 .setSize(sizeof(float))
                 .setUsage(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eRayTracingKHR),
             VMA_MEMORY_USAGE_CPU_TO_GPU,
             context.device, context.allocator);
    }
}

void Renderer::update_uniforms(uint32_t image_id, Scene& scene)
{
    //std::cout << scene.time << std::endl;
    m_time_uniforms[image_id].copy(reinterpret_cast<void*>(&scene.time), sizeof(float));
}

void Renderer::create_storage_image(Context& context)
{
    // One image per swapchain image?
    m_storage_image = Allocated_image(
        vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setExtent(vk::Extent3D(m_swapchain.extent, 1))
        .setMipLevels(1u)
        .setArrayLayers(1u)
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setTiling(vk::ImageTiling::eOptimal)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage),
        VMA_MEMORY_USAGE_GPU_ONLY,
        m_device, context.allocator);

    m_image_view = m_device.createImageView(vk::ImageViewCreateInfo()
        .setImage(m_storage_image.image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSubresourceRange(vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0u)
            .setLevelCount(1u)
            .setBaseArrayLayer(0u)
            .setLayerCount(1u)));

    One_time_command_buffer command_buffer(m_device, context.command_pool, context.graphics_queue);

    auto barrier = vk::ImageMemoryBarrier()
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(m_storage_image.image)
        .setSrcAccessMask({})
        .setDstAccessMask({}) // vk::AccessFlagBits::eColorAttachmentWrite ?
        .setSubresourceRange(vk::ImageSubresourceRange()
            .setLevelCount(1u)
            .setLayerCount(1)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setAspectMask(vk::ImageAspectFlagBits::eColor));
    command_buffer.command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,  // All command?
        {}, {}, {}, barrier);
    command_buffer.submit_and_wait_idle();
}

void Renderer::create_descriptor_sets(Scene& scene)
{
    // TODO set one for each swapchain image
    std::array pool_sizes
    {
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eAccelerationStructureKHR)
            .setDescriptorCount(Swapchain::image_count),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eStorageImage)
            .setDescriptorCount(Swapchain::image_count),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(Swapchain::image_count),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(Swapchain::image_count)
    };
    m_descriptor_pool = m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo()
        .setPoolSizeCount(static_cast<uint32_t>(pool_sizes.size()))
        .setPPoolSizes(pool_sizes.data())
        .setMaxSets(Swapchain::image_count));

    std::array<vk::DescriptorSetLayout, Swapchain::image_count> layouts;
    std::fill(layouts.begin(), layouts.end(), m_pipeline.descriptor_set_layout);
    m_descriptor_sets = m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(m_descriptor_pool)
        .setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
        .setPSetLayouts(layouts.data()));

    for (size_t i = 0; i < Swapchain::image_count; i++)
    {
        auto descriptor_acceleration_structure_info = vk::WriteDescriptorSetAccelerationStructureKHR()
            .setAccelerationStructureCount(1u)
            .setPAccelerationStructures(&m_tlas.acceleration_structure);

        auto image_info = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(m_image_view);

        auto primitives_info = vk::DescriptorBufferInfo()
            .setBuffer(scene.primitives_buffer.buffer)
            .setOffset(0u)
            .setRange(VK_WHOLE_SIZE);

        auto time_uniform_info = vk::DescriptorBufferInfo()
            .setBuffer(m_time_uniforms[i].buffer)
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
    for (size_t i = 0; i < max_frames_in_flight; i++)
    {
        m_semaphore_available_in_flight[i] = m_device.createSemaphore({});
        m_semaphore_finished_in_flight[i] = m_device.createSemaphore({});
        m_fence_in_flight[i] = m_device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
        m_fence_swapchain_in_flight[i] = vk::Fence();
    }
}

void Renderer::reload_pipeline(Context& context)
{
    for (auto& command_buffer : m_command_buffers) {
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
    }
    m_pipeline.reload(context);
    create_command_buffers();
}

}*/