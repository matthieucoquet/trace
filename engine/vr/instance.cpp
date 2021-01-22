#include "instance.hpp"
#include <fmt/core.h>

//OPENXR_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
namespace xr { DispatchLoaderDynamic default_dispatch_loader_dynamic{}; }

static XrBool32 debug_callback(
    XrDebugUtilsMessageSeverityFlagsEXT /*message_severity*/,
    XrDebugUtilsMessageTypeFlagsEXT /*message_types*/,
    const XrDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* /*user_data*/) 
{
    fmt::print(callback_data->message);
    return XR_TRUE;
}

namespace sdf_editor::vr
{
constexpr bool verbose = true;

Instance::Instance()
{
    std::vector<const char*> required_extensions{
        XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,
        XR_KHR_CONVERT_TIMESPEC_TIME_EXTENSION_NAME
    };

#ifdef TRACE_USE_DYNAMIC_LOADER
    auto xrGetInstanceProcAddr = m_dynamic_loader.getProcAddress<PFN_xrGetInstanceProcAddr>("xrGetInstanceProcAddr");
    OPENXR_HPP_DEFAULT_DISPATCHER.populateBase(xrGetInstanceProcAddr);
#endif

    if constexpr (verbose) {
        {
            fmt::print("OpenXR API layers:\n");
            auto properties = xr::enumerateApiLayerPropertiesToVector(
#ifndef TRACE_USE_DYNAMIC_LOADER
                xr::DispatchLoaderStatic()
#endif
            );
            for (const auto& property : properties) {
                fmt::print("\t{}\n", property.layerName);
            }
        }
        {
            fmt::print("Available OpenXR instance extensions:\n");
            auto properties = xr::enumerateInstanceExtensionPropertiesToVector(nullptr
#ifndef TRACE_USE_DYNAMIC_LOADER
                , xr::DispatchLoaderStatic()
#endif
            );
            for (const auto& property : properties) {
                fmt::print("\t{}\n", property.extensionName);
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
    }
#ifndef TRACE_USE_DYNAMIC_LOADER
        , xr::DispatchLoaderStatic()
#endif
    );
    //OPENXR_HPP_DEFAULT_DISPATCHER.populateFully(instance);
    xr::default_dispatch_loader_dynamic = xr::DispatchLoaderDynamic(instance);
    xr::default_dispatch_loader_dynamic.populateFully();

    system_id = instance.getSystem(xr::SystemGetInfo{
        .formFactor = xr::FormFactor::HeadMountedDisplay
        });

    if constexpr (verbose) {
        auto instance_properties = instance.getInstanceProperties();
        fmt::print("Runtime: \n\t{}\n", instance_properties.runtimeName);
        auto system_properties = instance.getSystemProperties(system_id);
        fmt::print("HMD: \n\t{}\n", system_properties.systemName);
    }

    auto view_configurations = instance.enumerateViewConfigurationsToVector(system_id);
    if (std::none_of(view_configurations.cbegin(), view_configurations.cend(), [](const auto& config) {
        return config == xr::ViewConfigurationType::PrimaryStereo;
    })) {
        throw std::runtime_error("Failed to find a stereo HMD.");
    }
    auto blend_modes = instance.enumerateEnvironmentBlendModesToVector(system_id, xr::ViewConfigurationType::PrimaryStereo);
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
        fmt::print("Max supported: \n\t{}.{}\n", vulkan_requirements.maxApiVersionSupported.major(), vulkan_requirements.maxApiVersionSupported.minor());
        //throw std::runtime_error("OpenXR doesn't support Vulkan 1.2.");
    }
}

Instance::~Instance()
{
    if (instance) {
        instance.destroy();
    }
}

void Instance::split_and_append(char* new_extensions, std::vector<const char*>& required_extensions) const
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

float Instance::mirror_recommended_ratio() const
{
    auto view_configuration_views = instance.enumerateViewConfigurationViewsToVector(system_id, xr::ViewConfigurationType::PrimaryStereo);
    return static_cast<float>(view_configuration_views[0].recommendedImageRectWidth) / view_configuration_views[0].recommendedImageRectHeight;
}

}