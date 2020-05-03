#pragma once
#include "common.h"
#include "allocation.h"
#include <filesystem>

class Context;

class Raytracing_pipeline
{
public:
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    vk::PhysicalDeviceRayTracingPropertiesKHR raytracing_properties;
    vk::DescriptorSetLayout descriptor_set_layout;
    Allocated_buffer shader_binding_table{};

    Raytracing_pipeline(Context& context);
    Raytracing_pipeline(const Raytracing_pipeline& other) = delete;
    Raytracing_pipeline(Raytracing_pipeline&& other) = delete;
    Raytracing_pipeline& operator=(const Raytracing_pipeline& other) = default;
    Raytracing_pipeline& operator=(Raytracing_pipeline&& other) = default;
    ~Raytracing_pipeline();
private:
    vk::Device m_device;

    void create_shader_binding_table(Context& context, uint32_t group_count);  // TODO should be here ?
    vk::ShaderModule shader_module_from_file(std::filesystem::path path) const;
};