#pragma once

#include "vr_common.hpp"
#ifdef TRACE_USE_DYNAMIC_LOADER
#include <openxr/openxr_dynamic_loader.hpp>
#endif

namespace sdf_editor::vr
{

class Instance
{
public:
    xr::Instance instance;
    xr::SystemId system_id;
    //xr::DispatchLoaderDynamic dynamic_dispatch;

    Instance();
    Instance(const Instance& other) = delete;
    Instance(Instance&& other) = delete;
    Instance& operator=(Instance& other) = delete;
    Instance& operator=(Instance&& other) = delete;
    ~Instance();

    void split_and_append(char* new_extensions, std::vector<const char*>& required_extensions) const;
    float mirror_recommended_ratio() const;
    void create_vulkan_instance(vk::Instance& vk_instance, vk::InstanceCreateInfo create_info, PFN_vkGetInstanceProcAddr instance_proc_addr);
    void create_vulkan_device(vk::Device& vk_device, vk::PhysicalDevice physical_device, vk::DeviceCreateInfo create_info, PFN_vkGetInstanceProcAddr instance_proc_addr);
private:
#ifdef TRACE_USE_DYNAMIC_LOADER
    xr::DynamicLoader m_dynamic_loader;
#endif
};

}