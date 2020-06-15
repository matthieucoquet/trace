#pragma once
#include "vk_common.h"

namespace vulkan
{

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

class Command_buffer_set
{
public:
    size_t size;
    std::vector<vk::CommandBuffer> command_buffers;
    std::vector<vk::Fence> fences;

    Command_buffer_set(vk::Device device, vk::CommandPool command_pool, vk::Queue queue, size_t buffer_size) :
        size(buffer_size),
        m_device(device),
        m_command_pool(command_pool),
        m_queue(queue)
    {
        m_command_buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
            .setCommandPool(command_pool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(size));
        //command_buffer = command_buffers.front();
        //command_buffer.begin(vk::CommandBufferBeginInfo()
        //    .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        for(size_t i = 0; i < size; i++)
        {
            m_fences[i] = m_device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
        }
    }

    ~Command_buffer_set()
    {
        for (size_t i = 0; i < size; i++)
        {
            m_device.destroyFence(m_fences[i]);
        }
        m_device.freeCommandBuffers(m_command_pool, m_command_buffers);
    }

    size_t find_available()
    {
        bool first = false;
        while (true)
        {
            for (size_t i = 0; i < size; i++)
            {
                if (device.getFenceStatus(m_fences[i]) == success) {
                    return i;
                }
            }
            if (first) {
                first = false;
                std::cout << "Warning: no command buffer available, waiting until one get available."
            }
        }
    }

private:
    vk::Device m_device;
    vk::CommandPool m_command_pool;
    vk::Queue m_queue;
};

}