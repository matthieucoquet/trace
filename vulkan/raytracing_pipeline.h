#pragma once
#include "vk_common.h"
#include "allocation.h"
#include <filesystem>

class Scene;

namespace vulkan
{

class Context;

class Raytracing_pipeline
{
public:
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    vk::PhysicalDeviceRayTracingPropertiesKHR raytracing_properties;
    vk::DescriptorSetLayout descriptor_set_layout;
    Allocated_buffer shader_binding_table{};
    vk::DeviceSize offset_raygen_side_group;
    vk::DeviceSize offset_miss_group;
    vk::DeviceSize offset_hit_group;

    Raytracing_pipeline(Context& context, Scene& scene);
    Raytracing_pipeline(const Raytracing_pipeline& other) = delete;
    Raytracing_pipeline(Raytracing_pipeline&& other) = delete;
    Raytracing_pipeline& operator=(const Raytracing_pipeline& other) = default;
    Raytracing_pipeline& operator=(Raytracing_pipeline&& other) = default;
    ~Raytracing_pipeline();

    void reload(Context& context);
private:
    vk::Device m_device;

    void create_shader_binding_table(Context& context, uint32_t group_count);  // TODO should be here ?
    void create_pipeline(Context& context, Scene& scene);
};

}