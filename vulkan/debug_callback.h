#pragma once
#include "common.h"
#include <iostream>

VkBool32 debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    VkDebugUtilsMessengerCallbackDataEXT const* callback_data,
    void* /*pUserData*/)
{
    std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity)) << ": " << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_type)) << std::endl;
    std::cerr << "\t" << "message:" << callback_data->pMessage << std::endl;
    if (0 < callback_data->queueLabelCount)
    {
        std::cerr << "\t" << "Queue Labels:" << std::endl;
        for (uint8_t i = 0; i < callback_data->queueLabelCount; i++)
        {
            std::cerr << "\t\t" << "labelName: " << callback_data->pQueueLabels[i].pLabelName << std::endl;
        }
    }
    if (0 < callback_data->cmdBufLabelCount)
    {
        std::cerr << "\t" << "CommandBuffer Labels:" << std::endl;
        for (uint8_t i = 0; i < callback_data->cmdBufLabelCount; i++)
        {
            std::cerr << "\t\t" << "labelName: " << callback_data->pCmdBufLabels[i].pLabelName << std::endl;
        }
    }
    if (0 < callback_data->objectCount)
    {
        std::cerr << "\t" << "Objects:\n";
        for (uint8_t i = 0; i < callback_data->objectCount; i++)
        {
            std::cerr << "\t\t\t" << "objectType: " << vk::to_string(static_cast<vk::ObjectType>(callback_data->pObjects[i].objectType)) << std::endl;
            std::cerr << "\t\t\t" << "objectHandle: " << callback_data->pObjects[i].objectHandle << std::endl;
            if (callback_data->pObjects[i].pObjectName)
            {
                std::cerr << "\t\t\t" << "objectName: " << callback_data->pObjects[i].pObjectName << std::endl;
            }
        }
    }
    return false;
}