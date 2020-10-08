#pragma once
#include "vk_common.hpp"
#include "vma_buffer.hpp"
#include <filesystem>

struct Scene;

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
    Vma_buffer shader_binding_table{};

    vk::DeviceSize offset_raygen_side_group;
    vk::DeviceSize offset_miss_group;
    vk::DeviceSize offset_hit_group;
    size_t nb_group_miss;
    size_t nb_group_primary;

    Raytracing_pipeline(Context& context, Scene& scene);
    Raytracing_pipeline(const Raytracing_pipeline& other) = delete;
    Raytracing_pipeline(Raytracing_pipeline&& other) = delete;
    Raytracing_pipeline& operator=(const Raytracing_pipeline& other) = default;
    Raytracing_pipeline& operator=(Raytracing_pipeline&& other) = default;
    ~Raytracing_pipeline();

    void create_pipeline(Scene& scene);
private:
    vk::Device m_device;

    void create_shader_binding_table(Context& context, uint32_t group_count);
};

}