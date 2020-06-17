#include "Desktop_mirror.h"
#include "context.h"
#include "command_buffer.h"
#include <iostream>
namespace vulkan
{

Desktop_mirror::Desktop_mirror(Context& context, size_t size) :
    m_device(context.device),
    m_queue(context.graphics_queue),
    m_swapchain(context)
{
    create_synchronization(size);
}

Desktop_mirror::~Desktop_mirror()
{
    for (size_t i = 0; i < m_semaphore_available.size(); i++)
    {
        m_device.destroySemaphore(m_semaphore_available[i]);
        m_device.destroySemaphore(m_semaphore_finished[i]);
    }
}

void Desktop_mirror::copy(vk::CommandBuffer& command_buffer, vk::Image vr_image, size_t current_id, vk::Extent2D extent)
{
    auto acquire_result = m_device.acquireNextImageKHR(m_swapchain.swapchain, 0, m_semaphore_available[current_id], {});
    if (acquire_result.result == vk::Result::eErrorOutOfDateKHR) {
        assert(false);
    }
    if (acquire_result.result == vk::Result::eTimeout) {
        std::cout << "Desktop acquireNextImageKHR timeout." << std::endl;
        assert(false);
    }
    m_image_id = acquire_result.value;

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eTransfer,
        {}, {}, {},
        vk::ImageMemoryBarrier()
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(m_swapchain.images[m_image_id])
        .setSrcAccessMask({})
        .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setSubresourceRange(vk::ImageSubresourceRange()
            .setLevelCount(1u)
            .setLayerCount(1)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setAspectMask(vk::ImageAspectFlagBits::eColor)));

    command_buffer.blitImage(
        vr_image,
        vk::ImageLayout::eTransferSrcOptimal,
        m_swapchain.images[m_image_id],
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageBlit()
        .setSrcOffsets({ vk::Offset3D(0, 0, 0), vk::Offset3D(extent.width / 2, extent.height, 1) })
        .setSrcSubresource(vk::ImageSubresourceLayers()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setMipLevel(0u)
            .setLayerCount(1u)
            .setBaseArrayLayer(0u))
        .setDstOffsets({ vk::Offset3D(0, 0, 0), vk::Offset3D(m_swapchain.extent.width, m_swapchain.extent.height, 1) })
        .setDstSubresource(vk::ImageSubresourceLayers()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setMipLevel(0u)
            .setLayerCount(1u)
            .setBaseArrayLayer(0u)),
        vk::Filter::eLinear
    );

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {}, {}, {},
        vk::ImageMemoryBarrier()
        .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(m_swapchain.images[m_image_id])
        .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask({})
        .setSubresourceRange(vk::ImageSubresourceRange()
            .setLevelCount(1u)
            .setLayerCount(1)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setAspectMask(vk::ImageAspectFlagBits::eColor)));
}

void Desktop_mirror::present(vk::CommandBuffer& command_buffer, vk::Fence fence, size_t current_id)
{
    vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eTransfer;
    m_queue.submit(
        vk::SubmitInfo()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&m_semaphore_available[current_id])
        .setPWaitDstStageMask(&wait_stages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&command_buffer)
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&m_semaphore_finished[current_id]),
        fence);

    auto present_result = m_queue.presentKHR(vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&m_semaphore_finished[current_id])
        .setSwapchainCount(1)
        .setPSwapchains(&m_swapchain.swapchain)
        .setPImageIndices(&m_image_id));
    if (present_result == vk::Result::eErrorOutOfDateKHR) {
        assert(false);
    }
}


void Desktop_mirror::create_synchronization(size_t size)
{
    m_semaphore_available.reserve(size);
    m_semaphore_finished.reserve(size);
    for (size_t i = 0; i < size; i++)
    {
        m_semaphore_available.push_back(m_device.createSemaphore({}));
        m_semaphore_finished.push_back(m_device.createSemaphore({}));
    }
}

}