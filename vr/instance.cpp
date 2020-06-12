#include "instance.h"

#include <iostream>

OPENXR_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static XrBool32 debug_callback(
    XrDebugUtilsMessageSeverityFlagsEXT /*message_severity*/,
    XrDebugUtilsMessageTypeFlagsEXT /*message_types*/,
    const XrDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* /*user_data*/) 
{
    std::cout << callback_data->message << std::endl;
    return XR_TRUE;
}

namespace vr
{
constexpr bool verbose = true;

Instance::Instance()
{
    std::vector<const char*> required_extensions{
        XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
    };
    //auto xrGetInstanceProcAddr = m_dynamic_loader.getProcAddress<PFN_xrGetInstanceProcAddr>("xrGetInstanceProcAddr");
    //OPENXR_HPP_DEFAULT_DISPATCHER.populateBase(xrGetInstanceProcAddr);

    if constexpr (verbose) {
        {
            std::cout << "API layers:" << std::endl;
            auto properties = xr::enumerateApiLayerProperties(xr::DispatchLoaderStatic());
            for (const auto& property : properties) {
                std::cout << "\t" << property.layerName << std::endl;
            }
        }
        {
            std::cout << "Available instance extensions:" << std::endl;
            auto properties = xr::enumerateInstanceExtensionProperties(nullptr, xr::DispatchLoaderStatic());
            for (const auto& property : properties) {
                std::cout << "\t" << property.extensionName << std::endl;
            }
        }
    }

    xr::DebugUtilsMessengerCreateInfoEXT dumci;
    dumci.messageSeverities = xr::DebugUtilsMessageSeverityFlagBitsEXT::AllBits;
    dumci.messageTypes = xr::DebugUtilsMessageTypeFlagBitsEXT::AllBits;
    dumci.userData = nullptr;
    dumci.userCallback = &debug_callback;

    instance = xr::createInstance(xr::InstanceCreateInfo{
        .next = nullptr,
        //.next = &dumci,
        .applicationInfo = {
            .applicationName = "trace",
            .applicationVersion = 0,
            .engineName = {},
            .engineVersion = 0,
            .apiVersion = xr::Version::current() },
        .enabledApiLayerCount = 0,
        .enabledApiLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
        .enabledExtensionNames = required_extensions.data()
    }, xr::DispatchLoaderStatic());
    OPENXR_HPP_DEFAULT_DISPATCHER.populateFully(instance);

    system_id = instance.getSystem(xr::SystemGetInfo{
        .formFactor = xr::FormFactor::HeadMountedDisplay
        });

    if constexpr (verbose) {
        auto instance_properties = instance.getInstanceProperties();
        std::cout << "Runtime: \n\t" << instance_properties.runtimeName << std::endl;
        auto system_properties = instance.getSystemProperties(system_id);
        std::cout << "HMD: \n\t" << system_properties.systemName << std::endl;
    }

    auto view_configurations = instance.enumerateViewConfigurations(system_id);
    if (std::none_of(view_configurations.cbegin(), view_configurations.cend(), [](const auto& config) {
        return config == xr::ViewConfigurationType::PrimaryStereo;
    })) {
        throw std::runtime_error("Failed to find a stereo HMD.");
    }
    auto blend_modes = instance.enumerateEnvironmentBlendModes(system_id, xr::ViewConfigurationType::PrimaryStereo);
    if (std::none_of(blend_modes.cbegin(), blend_modes.cend(), [](const auto& mode) {
        return mode == xr::EnvironmentBlendMode::Opaque;
    })) {
        throw std::runtime_error("Failed to find a HMD with opaque blend mode.");
    }

    xr::GraphicsRequirementsVulkanKHR vulkan_requirements = instance.getVulkanGraphicsRequirementsKHR(system_id);
    if (vulkan_requirements.minApiVersionSupported.major() == 1 && vulkan_requirements.minApiVersionSupported.minor() > 2) {
        throw std::runtime_error("Vulkan version is too old for OpenXR.");
    }
    if (vulkan_requirements.maxApiVersionSupported.major() == 1 && vulkan_requirements.maxApiVersionSupported.minor() < 2) {
        std::cout << "Max supported: \n\t" << vulkan_requirements.maxApiVersionSupported.major() << "." << vulkan_requirements.maxApiVersionSupported.minor() << std::endl;
        //throw std::runtime_error("OpenXR doesn't support Vulkan 1.2.");
    }
}

Instance::~Instance()
{
    if (instance) {
        instance.destroy();
    }
}

void Instance::splitAndAppend(char* new_extensions, std::vector<const char*>& required_extensions) const
{
    char* next_extension = new_extensions;
    while (next_extension) {
        next_extension = strchr(new_extensions, ' ');
        if (next_extension) {
            *next_extension = '\0';
        }
        // Not required but we make sure there is no duplicate
        if (std::none_of(required_extensions.cbegin(), required_extensions.cend(), [new_extensions](const auto& added_extension) {
            return strcmp(added_extension, new_extensions) == 0;
        })) {
            required_extensions.push_back(new_extensions);
        }
        if (next_extension) {
            new_extensions = next_extension + 1;
        }
    }
}

}