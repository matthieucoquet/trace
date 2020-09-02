#pragma once
#include "vk_common.hpp"

class Window;
namespace vr {
class Instance;
}

namespace vulkan
{

class Context
{
public:
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::Device device;
    vk::PhysicalDevice physical_device;
    vk::CommandPool command_pool;  // Is it optimal to have one global pool here ?
    uint32_t queue_family = 0u;
    vk::Queue graphics_queue;
    VmaAllocator allocator;
    vk::DescriptorPool descriptor_pool;

    Context(Window& window, vr::Instance& vr_instance);
    Context(const Context& other) = delete;
    Context(Context&& other) = delete;
    Context& operator=(const Context& other) = delete;
    Context& operator=(Context&& other) = delete;
    ~Context();

private:
    vk::DynamicLoader m_dynamic_loader;
    vk::DebugUtilsMessengerEXT m_debug_messenger{};

    void init_instance(Window& window, vr::Instance& vr_instance);
    void init_device(vr::Instance& vr_instance);
    void init_allocator();
    void init_descriptor_pool();
};

}