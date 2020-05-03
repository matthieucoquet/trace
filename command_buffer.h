#pragma once
#include "common.h"

class One_time_command_buffer
{
public:
    vk::CommandBuffer command_buffer;

    One_time_command_buffer(vk::Device device, vk::CommandPool command_pool, vk::Queue queue) :
        m_device(device),
        m_command_pool(command_pool),
        m_queue(queue)
    {
        std::vector<vk::CommandBuffer> command_buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
            .setCommandPool(command_pool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1));
        command_buffer = command_buffers.front();
        command_buffer.begin(vk::CommandBufferBeginInfo()
            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    }

    // Would be better to set a fence for synchronisation
    void submit_and_wait_idle()
    {
        command_buffer.end();
        m_queue.submit(vk::SubmitInfo()
            .setCommandBufferCount(1)
            .setPCommandBuffers(&command_buffer), {});
        m_queue.waitIdle();
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
    }
protected:
    vk::Device m_device;
    vk::CommandPool m_command_pool;
    vk::Queue m_queue;
};
