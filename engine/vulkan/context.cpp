#define VMA_IMPLEMENTATION  // VMA implementation: Need to be define before any header
#include "context.hpp"
#include "engine/window.hpp"
#include "vr/instance.hpp"
#include "debug_callback.hpp"

#include <fmt/core.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace sdf_editor::vulkan
{

constexpr bool verbose = false;
constexpr bool sdk_available = true;

Context::Context(Window& window, vr::Instance* vr_instance)
{
    init_instance(window, vr_instance);
    surface = window.create_surface(instance);
    init_device(vr_instance);
    init_allocator();
    init_descriptor_pool();
}

Context::~Context()
{
    vmaDestroyAllocator(allocator);
    device.destroyDescriptorPool(descriptor_pool);
    device.destroyCommandPool(command_pool);
    device.destroy();
    instance.destroySurfaceKHR(surface);
    instance.destroyDebugUtilsMessengerEXT(m_debug_messenger);
    instance.destroy();
}

void Context::init_instance(Window& window, vr::Instance* vr_instance)
{
    auto required_extensions = window.required_extensions();
    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifndef VR_USE_VULKAN2
    // Warning: vr_required_extensions hold the memory to the string, should not be destroyed until createInstance
    std::string vr_required_extensions{};
    if (vr_instance)
    {
        vr_required_extensions = vr_instance->instance.getVulkanInstanceExtensionsKHR(vr_instance->system_id);
        if (!vr_required_extensions.empty()) {
            vr_instance->split_and_append(vr_required_extensions.data(), required_extensions);
        }
    }
#endif

    std::array required_instance_layers{ "VK_LAYER_KHRONOS_validation" };

    // Find Vulkan dynamic lib and fetch needed functions to create instance
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = m_dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    if constexpr (verbose) {
        fmt::print("Instance layers:\n");
        for (const auto& property : vk::enumerateInstanceLayerProperties()) {
            fmt::print("\t{}\n", property.layerName);
        }
        fmt::print("Instance extensions:\n");
        for (const auto& property : vk::enumerateInstanceExtensionProperties()) {
            fmt::print("\t{}\n", property.extensionName);
        }
        fmt::print("Required instance extensions:\n");
        for (const auto& required_extension : required_extensions) {
            fmt::print("\t{}\n", required_extension);
        }
    }

    std::array validation_features{ vk::ValidationFeatureEnableEXT::eBestPractices };
    //vk::ValidationFeatureEnableEXT::eGpuAssisted,
    //vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot };
    vk::ValidationFeaturesEXT validation_features_ext{
        .enabledValidationFeatureCount = static_cast<uint32_t>(validation_features.size()),
        .pEnabledValidationFeatures = validation_features.data()
    };
    vk::DebugUtilsMessengerCreateInfoEXT debug_create_info{
        //.pNext = sdk_available ? &validation_features : nullptr,
        .pNext = nullptr,
        .messageSeverity = /*vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |*/ vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        /*vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |*/ vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
    .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
    .pfnUserCallback = &debug_callback
    };


    vk::ApplicationInfo app_info {
        .pApplicationName = "trace",
        .apiVersion = VK_API_VERSION_1_2
    };
    vk::InstanceCreateInfo create_info{
    .pNext = nullptr, //&debug_create_info,
    .pApplicationInfo = &app_info,
    //.enabledLayerCount = static_cast<uint32_t>(required_instance_layers.size()),
    //.ppEnabledLayerNames = required_instance_layers.data(),
    .enabledLayerCount = static_cast<uint32_t>(sdk_available ? required_instance_layers.size() : 0u),
    .ppEnabledLayerNames = sdk_available ? required_instance_layers.data() : nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
    .ppEnabledExtensionNames = required_extensions.data()
    };
#ifdef VR_USE_VULKAN2
    if (vr_instance) {
        vr_instance->create_vulkan_instance(instance, create_info, vk::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr);
    }
#else
    instance = vk::createInstance(create_info);
#endif 

    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    m_debug_messenger = instance.createDebugUtilsMessengerEXT(debug_create_info);
}

void Context::init_device(vr::Instance* vr_instance)
{
    std::vector required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
        // VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME
    };

#ifdef USING_AFTERMATH
    required_device_extensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
    required_device_extensions.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
    // required_device_extensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
#endif

    std::vector<vk::PhysicalDevice> potential_physical_devices;
    std::string vr_required_extensions{};
    if (vr_instance)
    {
#ifdef VR_USE_VULKAN2
        vk::PhysicalDevice xr_physical_device{ vr_instance->instance.getVulkanGraphicsDevice2KHR(xr::VulkanGraphicsDeviceGetInfoKHR{
                .systemId = vr_instance->system_id,
                .vulkanInstance = instance
        }) };
        potential_physical_devices.push_back(xr_physical_device);
#else
        vr_required_extensions = vr_instance->instance.getVulkanDeviceExtensionsKHR(vr_instance->system_id);
        if (!vr_required_extensions.empty()) {
            vr_instance->split_and_append(vr_required_extensions.data(), required_device_extensions);
        }
        if (strcmp(required_device_extensions.back(), "VK_EXT_debug_marker") == 0) {
            required_device_extensions.pop_back();
        }
        potential_physical_devices.push_back(vr_instance->instance.getVulkanGraphicsDeviceKHR(vr_instance->system_id, instance));
#endif
    }
    else
    {
        potential_physical_devices = instance.enumeratePhysicalDevices();
    }

    for (const auto& potential_physical_device : potential_physical_devices)
    {
        auto properties = potential_physical_device.getProperties();
        if constexpr (verbose) {
            fmt::print("Found GPU: \n\t{}\n", properties.deviceName);
        }

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
            auto as_features = vk::PhysicalDeviceAccelerationStructureFeaturesKHR{ .pNext = &vulkan_12_features };
            auto pipeline_features = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR{ .pNext = &as_features };
            auto features = vk::PhysicalDeviceFeatures2{ .pNext = &pipeline_features };
            potential_physical_device.getFeatures2(&features);
            if (!pipeline_features.rayTracingPipeline || !pipeline_features.rayTraversalPrimitiveCulling)
                continue;
            if (!as_features.accelerationStructure /*|| !as_features.descriptorBindingAccelerationStructureUpdateAfterBind*/)
                continue;
            if (!vulkan_12_features.bufferDeviceAddress || !vulkan_12_features.uniformBufferStandardLayout || !vulkan_12_features.scalarBlockLayout ||
                !vulkan_12_features.uniformAndStorageBuffer8BitAccess)
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
            fmt::print("Devices extensions:\n");
            for (const auto& property : available_extensions) {
                fmt::print("\t{}\n", property.extensionName);
            }
            fmt::print("Required device extensions:\n");
            for (const auto& required_extension : required_device_extensions) {
                fmt::print("\t{}\n", required_extension);
            }
        }

        // Create device now that we found a suitable gpu 
        physical_device = potential_physical_device;
        float queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_create_info {
            .queueFamilyIndex = queue_family,
            .queueCount = 1u,
            .pQueuePriorities = &queue_priority
        };

        vk::PhysicalDeviceVulkan12Features vulkan_12_features {
            .uniformAndStorageBuffer8BitAccess = true,
            .scalarBlockLayout = true,
            .uniformBufferStandardLayout = true,
            .bufferDeviceAddress = true
        };
#ifdef USING_AFTERMATH
        vk::DeviceDiagnosticsConfigCreateInfoNV aftermath_features{
            .flags =
                vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableAutomaticCheckpoints |
                vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableResourceTracking |
                vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderDebugInfo
        };
        vulkan_12_features.pNext = &aftermath_features;
#endif

        vk::PhysicalDeviceAccelerationStructureFeaturesKHR raytracing_as_features{
            .pNext = &vulkan_12_features,
            .accelerationStructure = true
        };
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_pileline_features {
            .pNext = &raytracing_as_features,
            .rayTracingPipeline = true,
            .rayTraversalPrimitiveCulling = true,
        };
        vk::PhysicalDeviceFeatures2 device_features {
            .pNext = &raytracing_pileline_features,
            .features = {
                .shaderStorageImageMultisample = true,
            }
        };

        vk::DeviceCreateInfo create_info{
            .pNext = &device_features,  // Using pNext instead of pEnabledFeatures to enable raytracing
            .queueCreateInfoCount = 1u,
            .pQueueCreateInfos = &queue_create_info,
            .enabledExtensionCount = static_cast<uint32_t>(required_device_extensions.size()),
            .ppEnabledExtensionNames = required_device_extensions.data()
        };
#ifdef VR_USE_VULKAN2
        if (vr_instance) {
            vr_instance->create_vulkan_device(device, physical_device, create_info, vk::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr);
        }
#else
        device = physical_device.createDevice(create_info);
#endif
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

        graphics_queue = device.getQueue(queue_family, 0u);
        command_pool = device.createCommandPool(vk::CommandPoolCreateInfo{ .queueFamilyIndex = queue_family });
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

void Context::init_descriptor_pool()
{
    constexpr uint32_t max_swapchain_size = 10u;
    std::array pool_sizes
    {
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = max_swapchain_size },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eStorageImage,
            .descriptorCount = max_swapchain_size },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1 + max_swapchain_size },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = max_swapchain_size },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 2 }
    };
    descriptor_pool = device.createDescriptorPool(vk::DescriptorPoolCreateInfo{
        .maxSets = max_swapchain_size,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data()});
}

}