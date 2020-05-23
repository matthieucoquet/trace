#define VMA_IMPLEMENTATION  // VMA implementation: Need to be define before any header
#include "context.h"
#include "window.h"
#include "vr/context.h"
#include "debug_callback.h"

#include <iostream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vulkan
{

constexpr bool verbose = false;

Context::Context(Window& window, vr::Context* vr_context)
{
    init_instance(window, vr_context);
    surface = window.create_surface(instance);
    init_device(vr_context);
    init_allocator();
}

Context::~Context()
{
    vmaDestroyAllocator(allocator);
    device.destroyCommandPool(command_pool);
    device.destroy();
    instance.destroySurfaceKHR(surface);
    instance.destroyDebugUtilsMessengerEXT(m_debug_messenger);
    instance.destroy();
}

void Context::init_instance(Window& window, vr::Context* vr_context)
{
    auto required_extensions = window.required_extensions();
    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    if (vr_context)
    {
        auto vr_required_extensions = vr_context->instance.getVulkanInstanceExtensionsKHR(vr_context->system_id);
        if (!vr_required_extensions.empty()) {
            vr_context->splitAndAppend(vr_required_extensions.data(), required_extensions);
        }
    }

    std::array required_instance_layers{ "VK_LAYER_KHRONOS_validation" };

    // Find Vulkan dynamic lib and fetch needed functions to create instance
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = m_dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    if constexpr (verbose) {
        std::cout << "Instance extensions:" << std::endl;
        for (const auto& property : vk::enumerateInstanceExtensionProperties()) {
            std::cout << "\t" << property.extensionName << std::endl;
        }
        std::cout << "Required instance extensions:" << std::endl;
        for (const auto& required_extension : required_extensions) {
            std::cout << "\t" << required_extension << std::endl;
        }
    }

    auto debug_create_info = vk::DebugUtilsMessengerCreateInfoEXT()
        .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        .setPfnUserCallback(&debug_callback);
    std::array validation_features{ vk::ValidationFeatureEnableEXT::eBestPractices };
    auto validation_features_ext = vk::ValidationFeaturesEXT()
        .setPNext((VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info)
        .setEnabledValidationFeatureCount(static_cast<uint32_t>(validation_features.size()))
        .setPEnabledValidationFeatures(validation_features.data());

    auto app_info = vk::ApplicationInfo()
        .setPApplicationName("trace")
        .setApiVersion(VK_API_VERSION_1_2);
    instance = vk::createInstance(vk::InstanceCreateInfo()
        .setPApplicationInfo(&app_info)
        .setPNext(&validation_features_ext)
        .setPpEnabledExtensionNames(required_extensions.data())
        .setEnabledExtensionCount(static_cast<uint32_t>(required_extensions.size()))
        .setPpEnabledLayerNames(required_instance_layers.data())
        .setEnabledLayerCount(static_cast<uint32_t>(required_instance_layers.size())));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    m_debug_messenger = instance.createDebugUtilsMessengerEXT(debug_create_info);
}

void Context::init_device(vr::Context* vr_context)
{
    std::vector required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_EXTENSION_NAME,
        // The followings are required for VK_KHR_ray_tracing
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME
    };

    std::vector<vk::PhysicalDevice> potential_physical_devices;
    if (vr_context)
    {
        auto vr_required_extensions = vr_context->instance.getVulkanDeviceExtensionsKHR(vr_context->system_id);
        if (!vr_required_extensions.empty()) {
            vr_context->splitAndAppend(vr_required_extensions.data(), required_device_extensions);
        }

        potential_physical_devices.push_back(vr_context->instance.getVulkanGraphicsDeviceKHR(vr_context->system_id, instance));
    }
    else
    {
        potential_physical_devices = instance.enumeratePhysicalDevices();
    }

    for (const auto& potential_physical_device : potential_physical_devices)
    {
        auto properties = potential_physical_device.getProperties();
        if constexpr (verbose)
            std::cout << "Found GPU: \n\t" << properties.deviceName << std::endl;

        // For simplicity, we take only discrete gpu
        if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
            continue;

        // Check extensions availability
        auto available_extensions = potential_physical_device.enumerateDeviceExtensionProperties();
        if (std::any_of(required_device_extensions.cbegin(), required_device_extensions.cend(), [&available_extensions](const char* name) {
            return std::all_of(available_extensions.cbegin(), available_extensions.cend(), [name](const VkExtensionProperties& prop)
            {
                return strcmp(prop.extensionName, name) != 0;
            });
        }))
            continue;

        // Checking features
        {
            auto vulkan_12_features = vk::PhysicalDeviceVulkan12Features();
            auto raytracing_features = vk::PhysicalDeviceRayTracingFeaturesKHR().setPNext(&vulkan_12_features);
            auto features = vk::PhysicalDeviceFeatures2().setPNext(&raytracing_features);
            potential_physical_device.getFeatures2(&features);
            if (!raytracing_features.rayTracing)
                continue;
            if (!vulkan_12_features.bufferDeviceAddress || !vulkan_12_features.uniformBufferStandardLayout || !vulkan_12_features.scalarBlockLayout)
                continue;
        }

        // Check graphic and present queue family
        auto queue_families_properties = potential_physical_device.getQueueFamilyProperties();
        queue_family = 0u;
        for (const auto& property : queue_families_properties)
        {
            if (property.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer))
            {
                if (potential_physical_device.getSurfaceSupportKHR(queue_family, surface))
                    break;
            }
            queue_family++;
        }
        if (queue_family == queue_families_properties.size())
            continue;

        if constexpr (verbose) {
            std::cout << "Devices extensions:" << std::endl;
            for (const auto& property : available_extensions) {
                std::cout << "\t" << property.extensionName << std::endl;
            }
            std::cout << "Required device extensions:" << std::endl;
            for (const auto& required_extension : required_device_extensions) {
                std::cout << "\t" << required_extension << std::endl;
            }
        }

        // Create device now that we found a suitable gpu 
        physical_device = potential_physical_device;
        float queue_priority = 1.0f;
        auto queue_create_info = vk::DeviceQueueCreateInfo()
            .setQueueFamilyIndex(queue_family)
            .setQueueCount(1u)
            .setPQueuePriorities(&queue_priority);

        auto vulkan_12_features = vk::PhysicalDeviceVulkan12Features()
            .setBufferDeviceAddress(true)
            .setUniformBufferStandardLayout(true)
            .setScalarBlockLayout(true);
        auto raytracing_features = vk::PhysicalDeviceRayTracingFeaturesKHR()
            .setPNext(&vulkan_12_features)
            .setRayTracing(true);
        //.setRayTracingHostAccelerationStructureCommands(true);
        auto device_features = vk::PhysicalDeviceFeatures2()
            .setPNext(&raytracing_features)
            .setFeatures(vk::PhysicalDeviceFeatures()
                .setSamplerAnisotropy(true));
        device = physical_device.createDevice(vk::DeviceCreateInfo()
            .setPNext(&device_features)  // Using pNext instead of pEnabledFeatures to enable raytracing
            .setQueueCreateInfoCount(1u)
            .setPQueueCreateInfos(&queue_create_info)
            .setEnabledExtensionCount(static_cast<uint32_t>(required_device_extensions.size()))
            .setPpEnabledExtensionNames(required_device_extensions.data()));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

        graphics_queue = device.getQueue(queue_family, 0u);
        command_pool = device.createCommandPool(vk::CommandPoolCreateInfo()
            .setQueueFamilyIndex(queue_family));
        return;
    }
    throw std::runtime_error("Failed to find a suitable GPU.");
}

void Context::init_allocator()
{
    // We need to tell VMA the vulkan address from the dynamic dispatcher
    VmaVulkanFunctions vulkan_functions{
        .vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
        .vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
        .vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
        .vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
        .vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
        .vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
        .vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
        .vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
        .vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
        .vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
        .vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
        .vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2KHR,
        .vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2KHR,
        .vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2KHR
    };

    VmaAllocatorCreateInfo allocator_info{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physical_device,
        .device = device,
        .pVulkanFunctions = &vulkan_functions,
        .instance = instance
    };
    vmaCreateAllocator(&allocator_info, &allocator);
}

}