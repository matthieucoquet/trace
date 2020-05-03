#pragma once
#include "common.h"

class Window;

class Context
{
public:
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::Device device;
    vk::PhysicalDevice physical_device;
    vk::CommandPool command_pool;  // Is it optimal to have one global pool here ?
    vk::Queue graphics_queue;
    VmaAllocator allocator;

    Context(Window& window);
    Context(const Context& other) = delete;
    Context(Context&& other) = delete;
    Context& operator=(const Context& other) = delete;
    Context& operator=(Context&& other) = delete;
    ~Context();

private:
    vk::DynamicLoader m_dynamic_loader;
    vk::DebugUtilsMessengerEXT m_debug_messenger{};

    void init_instance(Window& window);
    void init_device();
    void init_allocator();
};