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
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_properties;
    vk::DescriptorSetLayout descriptor_set_layout;
    Vma_buffer shader_binding_table{};

    vk::DeviceSize shader_binding_table_stride;
    vk::DeviceSize offset_miss_group;
    vk::DeviceSize offset_hit_group;
    size_t nb_group_miss;
    size_t nb_group_primary;

    Raytracing_pipeline(Context& context, Scene& scene, vk::Sampler immutable_sampler);
    Raytracing_pipeline(const Raytracing_pipeline& other) = delete;
    Raytracing_pipeline(Raytracing_pipeline&& other) = delete;
    Raytracing_pipeline& operator=(const Raytracing_pipeline& other) = delete;
    Raytracing_pipeline& operator=(Raytracing_pipeline&& other) = delete;
    ~Raytracing_pipeline();

    void create_pipeline(Scene& scene);

    void update_shader_binding_table();
    std::vector<uint8_t> create_shader_binding_table();
private:
    vk::Device m_device;
};

}
