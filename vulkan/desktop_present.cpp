#include "desktop_present.h"
#include "context.h"
#include "command_buffer.h"

namespace vulkan
{

Desktop_present::Desktop_present(Context& context) :
    m_device(context.device),
    m_queue(context.graphics_queue),
    m_command_pool(context.command_pool),
    m_swapchain(context),
    m_blas(context, scene.aabbs_buffer.buffer, static_cast<uint32_t>(scene.aabbs.size())),
    m_tlas(context, m_blas, scene)
{
    create_command_buffers();
    create_synchronization();
}

Desktop_present::~Desktop_present()
{
    for (size_t i = 0; i < max_frames_in_flight; i++)
    {
        m_device.destroySemaphore(m_semaphore_available_in_flight[i]);
        m_device.destroySemaphore(m_semaphore_finished_in_flight[i]);
        //m_device.destroyFence(m_fence_in_flight[i]);
        // Don't need to destroy m_images_in_flight
    }
    //for (auto& command_buffer : m_command_buffers) {
    //    m_device.freeCommandBuffers(m_command_pool, command_buffer);
    //}
}

bool Desktop_present::present(vk::CommandBuffer& command_buffer, size_t current_id)
{
    auto wait_result = m_device.waitForFences(m_fence_in_flight[m_current_frame], true, 0);
    if (wait_result.result == vk::Result::eTimeout) {
        return false;
    }               
    auto acquire_result = m_device.acquireNextImageKHR(m_swapchain.swapchain, 0, m_semaphore_available[current_id], {});
    if (acquire_result.result == vk::Result::eErrorOutOfDateKHR) {
        assert(false);
    }
    if (acquire_result.result == vk::Result::eTimeout) {
        return false;
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
        .setPWaitSemaphores(&m_semaphore_available[current_id])
        .setPWaitDstStageMask(&wait_stages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&m_command_buffers[image_id])
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&m_semaphore_finished[current_id]),
        m_fence_in_flight[m_current_frame]);

    auto present_result = m_queue.presentKHR(vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&m_semaphore_finished[current_id])
        .setSwapchainCount(1)
        .setPSwapchains(&m_swapchain.swapchain)
        .setPImageIndices(&image_id));
    if (present_result == vk::Result::eErrorOutOfDateKHR) {
        assert(false);
    }
    //m_current_frame = (m_current_frame + 1u) % max_frames_in_flight;
}


void Renderer::create_synchronization(size_t size)
{
    m_semaphore_available.reserve(size);
    m_semaphore_finished.reserve(size);
    for (size_t i = 0; i < size; i++)
    {
        m_semaphore_available.push_back(m_device.createSemaphore({}));
        m_semaphore_finished.push_back(m_device.createSemaphore({}));
        //m_fence_in_flight[i] = m_device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
        //m_fence_swapchain_in_flight[i] = vk::Fence();
    }
}
/*
void Desktop_present::reload_pipeline(Context& context)
{
    for (auto& command_buffer : m_command_buffers) {
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
    }
    m_pipeline.reload(context);
    create_command_buffers();
}*/

}