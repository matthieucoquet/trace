#pragma once
#include "vk_common.h"
#include <fmt/core.h>
namespace vulkan
{

void print_queue_checkpoint_properties(vk::PhysicalDevice physical_device)
{
    uint32_t count = 0;
    physical_device.getQueueFamilyProperties2(&count, (vk::QueueFamilyProperties2*)nullptr);
    std::vector<vk::QueueFamilyProperties2> queues_properties(count);
    std::vector<vk::QueueFamilyCheckpointPropertiesNV> queues_checkpoint_properties(count);
    for (uint32_t i = 0; i < count; i++) {
        queues_properties[i].pNext = &queues_checkpoint_properties[i];
    }
    physical_device.getQueueFamilyProperties2(&count, queues_properties.data());

    for (auto& checkpoint_properties : queues_checkpoint_properties) {
        fmt::print("{}\n", vk::to_string(checkpoint_properties.checkpointExecutionStageMask));
    }
}

}