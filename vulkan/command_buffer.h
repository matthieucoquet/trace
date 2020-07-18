#pragma once
#include "vk_common.h"
#include <fmt/core.h>

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

class Reusable_command_buffers
{
public:
    size_t size;
    std::vector<vk::CommandBuffer> command_buffers;
    std::vector<vk::Fence> fences;

    Reusable_command_buffers(vk::Device device, vk::CommandPool command_pool, vk::Queue queue, size_t buffer_size) :
        size(buffer_size),
        m_device(device),
        m_command_pool(command_pool),
        m_queue(queue)
    {
        command_buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
            .setCommandPool(command_pool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(static_cast<uint32_t>(size)));
        fences.reserve(size);
        for(size_t i = 0; i < size; i++)
        {
            fences.push_back(m_device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled)));
        }
    }

    ~Reusable_command_buffers()
    {
        for (size_t i = 0; i < size; i++)
        {
            m_device.destroyFence(fences[i]);
        }
        m_device.freeCommandBuffers(m_command_pool, command_buffers);
    }

    size_t find_available()
    {
        bool first = false;
        while (true)
        {
            for (size_t i = 0; i < size; i++)
            {
                if (m_device.getFenceStatus(fences[i]) == vk::Result::eSuccess) {
                    m_device.resetFences(fences[i]);
                    command_buffers[i].reset({});
                    return i;
                }
            }
            if (first) {
                first = false;
                fmt::print("Warning: no command buffer available, waiting until one get available.\n");
            }
        }
    }

    void wait_until_done()
    {
        m_device.waitForFences(fences, true, std::numeric_limits<uint64_t>::max());
    }

private:
    vk::Device m_device;
    vk::CommandPool m_command_pool;
    vk::Queue m_queue;
};

}