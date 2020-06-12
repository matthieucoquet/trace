#pragma once
#include "vk_common.h"
#include "allocation.h"
#include <filesystem>
#include "shader_compile.h"

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

    Raytracing_pipeline(Context& context);
    Raytracing_pipeline(const Raytracing_pipeline& other) = delete;
    Raytracing_pipeline(Raytracing_pipeline&& other) = delete;
    Raytracing_pipeline& operator=(const Raytracing_pipeline& other) = default;
    Raytracing_pipeline& operator=(Raytracing_pipeline&& other) = default;
    ~Raytracing_pipeline();

    void reload(Context& context);
private:
    vk::Device m_device;
    Shader_compile shader_compiler;

    void create_shader_binding_table(Context& context, uint32_t group_count);  // TODO should be here ?
    void create_pipeline(Context& context);
};

}