#include "raytracing_pipeline.h"
#include "context.h"

#include <fstream>

namespace vulkan
{

Raytracing_pipeline::Raytracing_pipeline(Context& context) :
    m_device(context.device)
{
    vk::PhysicalDeviceProperties2 properties{};
    properties.pNext = &raytracing_properties;
    context.physical_device.getProperties2(&properties);

    std::array array_bindings
    {
        vk::DescriptorSetLayoutBinding()
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
            .setDescriptorCount(1u)
            .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR),
        vk::DescriptorSetLayoutBinding()
            .setBinding(1)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setDescriptorCount(1u)
            .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR),
        vk::DescriptorSetLayoutBinding()
            .setBinding(2)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1u)
            .setStageFlags(vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
        vk::DescriptorSetLayoutBinding()
            .setBinding(3)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1u)
            .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
    };
    descriptor_set_layout = m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(static_cast<uint32_t>(array_bindings.size()))
        .setPBindings(array_bindings.data()));

    pipeline_layout = m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo()
        .setPSetLayouts(&descriptor_set_layout)
        .setSetLayoutCount(1));

    create_pipeline(context);
}

Raytracing_pipeline::~Raytracing_pipeline()
{
    m_device.destroyPipeline(pipeline);
    m_device.destroyPipelineLayout(pipeline_layout);
    m_device.destroyDescriptorSetLayout(descriptor_set_layout);
}

void Raytracing_pipeline::create_shader_binding_table(Context& context, uint32_t group_count)
{
    uint32_t shader_binding_table_size = raytracing_properties.shaderGroupHandleSize * group_count;

    // Could be optimized by directly calling getRayTracingShaderGroupHandlesKHR between memory map/unmap
    std::vector<uint8_t> temp_buffer(shader_binding_table_size);
    m_device.getRayTracingShaderGroupHandlesKHR(pipeline, 0u, group_count, shader_binding_table_size, temp_buffer.data());

    shader_binding_table = Allocated_buffer(
        vk::BufferCreateInfo()
        .setSize(shader_binding_table_size)
        .setUsage(vk::BufferUsageFlagBits::eRayTracingKHR),
        temp_buffer.data(),
        context.device, context.allocator, context.command_pool, context.graphics_queue);
}

void Raytracing_pipeline::create_pipeline(Context& context)
{
    auto sphere_group = shader_compiler.compile_group(m_device, "sphere");
    auto cube_group = shader_compiler.compile_group(m_device, "cube");
    std::array shader_stages{
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eRaygenKHR)
            .setModule(shader_compiler.compile(m_device, "raygen.rgen", shaderc_raygen_shader))
            .setPName("main"),
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eMissKHR)
            .setModule(shader_compiler.compile(m_device, "miss.rmiss", shaderc_miss_shader))
            .setPName("main"),
        // Sphere
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eIntersectionKHR)
            .setModule(sphere_group.intersection)
            .setPName("main"),
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
            .setModule(sphere_group.closest_hit)
            .setPName("main"),
        // Cube
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eIntersectionKHR)
            .setModule(cube_group.intersection)
            .setPName("main"),
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
            .setModule(cube_group.closest_hit)
            .setPName("main"),
    };

    std::array groups{
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(0) // Raygen shader id
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(1) // miss shader id
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup)
            .setIntersectionShader(2) // intersection shader id
            .setClosestHitShader(3) // closest hit shader id
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup)
            .setIntersectionShader(4)  // intersection shader id
            .setClosestHitShader(5) // closest hit shader id
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
    };

    pipeline = m_device.createRayTracingPipelineKHR(
        nullptr,
        vk::RayTracingPipelineCreateInfoKHR()
        .setLayout(pipeline_layout)
        //.setFlags(vk::PipelineCreateFlagBits::eRayTracingSkipTrianglesKHR)
        .setStageCount(static_cast<uint32_t>(shader_stages.size()))
        .setPStages(shader_stages.data())
        .setGroupCount(static_cast<uint32_t>(groups.size()))
        .setPGroups(groups.data())
        .setMaxRecursionDepth(2)).value;

    create_shader_binding_table(context, static_cast<uint32_t>(groups.size()));

    for (auto& shader_stage : shader_stages) {
        m_device.destroyShaderModule(shader_stage.module);
    }
}

void Raytracing_pipeline::reload(Context& context)
{
    m_device.destroyPipeline(pipeline);
    create_pipeline(context);
}

}