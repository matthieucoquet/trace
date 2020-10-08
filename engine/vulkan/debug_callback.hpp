#pragma once
#include "vk_common.hpp"
#include <fmt/core.h>


VkBool32 debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    VkDebugUtilsMessengerCallbackDataEXT const* callback_data,
    void* /*pUserData*/)
{
    fmt::print("{} - {}:",
        vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_type)),
        vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity)));
    fmt::print("({} - {}) {}\n", callback_data->pMessageIdName, callback_data->messageIdNumber, callback_data->pMessage);
    if (callback_data->queueLabelCount > 0)
    {
        fmt::print("{} \n", "Queue Labels:");
        for (uint8_t i = 0; i < callback_data->queueLabelCount; i++)
        {
            fmt::print("\t{} \n", callback_data->pQueueLabels[i].pLabelName);
        }
    }
    if (callback_data->cmdBufLabelCount > 0)
    {
        fmt::print("{} \n", "CommandBuffer Labels:");
        for (uint8_t i = 0; i < callback_data->cmdBufLabelCount; i++)
        {
            fmt::print("\t{} \n", callback_data->pCmdBufLabels[i].pLabelName);
        }
    }
    if (callback_data->objectCount > 0)
    {
        fmt::print("{} \n", "Objects info:");
        for (uint8_t i = 0; i < callback_data->objectCount; i++)
        {
            fmt::print("\t({} - {}) {} \n",
                vk::to_string(static_cast<vk::ObjectType>(callback_data->pObjects[i].objectType)),
                callback_data->pObjects[i].objectHandle,
                callback_data->pObjects[i].pObjectName ? callback_data->pObjects[i].pObjectName : "");
        }
    }
    return false;
}