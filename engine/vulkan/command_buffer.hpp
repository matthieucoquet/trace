#pragma once
#include "vk_common.hpp"
#include <fmt/core.h>

namespace sdf_editor::vulkan
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
        std::vector<vk::CommandBuffer> command_buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
            .commandPool = command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1 });
        command_buffer = command_buffers.front();
        command_buffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    }

    // Would be better to set a fence for synchronisation
    void submit_and_wait_idle()
    {
        command_buffer.end();
        m_queue.submit(vk::SubmitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer }, {});
        m_queue.waitIdle();
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
    }
protected:
    vk::Device m_device;
    vk::CommandPool m_command_pool;
    vk::Queue m_queue;
};

class Reusable_command_pools
{
public:
    size_t size;
    std::vector<vk::CommandPool> command_pools;
    std::vector<vk::CommandBuffer> command_buffers;
    std::vector<vk::Fence> fences;
    vk::Device device;

    Reusable_command_pools(vk::Device device, uint32_t queue_family, size_t buffer_size) :
        size(buffer_size),
        device(device)
    {
        fences.reserve(size);
        command_pools.reserve(size);
        command_buffers.reserve(size);
        for(size_t i = 0; i < size; i++)
        {
            command_pools.push_back(device.createCommandPool(vk::CommandPoolCreateInfo{ .queueFamilyIndex = queue_family }));
            fences.push_back(device.createFence(vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled }));
            command_buffers.push_back(device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
                .commandPool = command_pools.back(),
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1 }).front()); // Only need one per pool right now
        }
    }

    ~Reusable_command_pools()
    {
        for (size_t i = 0; i < size; i++)
        {
            device.freeCommandBuffers(command_pools[i], command_buffers[i]);
            device.destroyCommandPool(command_pools[i]);
            device.destroyFence(fences[i]);
        }
    }

    size_t find_next()
    {
        bool first = false;
        while (true)
        {
            for (size_t i = 0; i < size; i++)
            {
                if (device.getFenceStatus(fences[i]) == vk::Result::eSuccess) {
                    device.resetFences(fences[i]);
                    device.resetCommandPool(command_pools[i], {});
                    return i;
                }
            }
            if (first) {
                first = false;
                fmt::print("Warning: no command pool available, waiting until one get available.\n");
            }
        }
    }

    void wait_until_done()
    {
        [[maybe_unused]] auto result = device.waitForFences(fences, true, std::numeric_limits<uint64_t>::max());
    }
};

}