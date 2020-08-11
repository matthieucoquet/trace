#include "raytracing_pipeline.h"
#include "context.h"
#include "core/scene.h"

#include <fstream>

namespace vulkan
{

Raytracing_pipeline::Raytracing_pipeline(Context& context, Scene& scene) :
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

    create_pipeline(context, scene);
}

Raytracing_pipeline::~Raytracing_pipeline()
{
    m_device.destroyPipeline(pipeline);
    m_device.destroyPipelineLayout(pipeline_layout);
    m_device.destroyDescriptorSetLayout(descriptor_set_layout);
}

void Raytracing_pipeline::create_shader_binding_table(Context& context, uint32_t group_count)
{
    auto find_alignement = [alignement = raytracing_properties.shaderGroupBaseAlignment](vk::DeviceSize offset) {
        auto ret = offset % alignement;
        return ret == 0 ? offset : offset + alignement - ret;
    };

    offset_raygen_side_group = find_alignement(1u * raytracing_properties.shaderGroupHandleSize);
    offset_miss_group = find_alignement(offset_raygen_side_group + 1u * raytracing_properties.shaderGroupHandleSize);
    offset_hit_group = find_alignement(offset_miss_group + 1u * raytracing_properties.shaderGroupHandleSize);

    auto shader_binding_table_size = raytracing_properties.shaderGroupHandleSize * group_count;
    auto shader_binding_table_size_aligned = static_cast<uint32_t>(offset_hit_group + raytracing_properties.shaderGroupHandleSize * (group_count - 2u));

    std::vector<uint8_t> temp_buffer(shader_binding_table_size);
    m_device.getRayTracingShaderGroupHandlesKHR(pipeline, 0u, group_count, shader_binding_table_size, temp_buffer.data());

    // Could be optimized by directly calling getRayTracingShaderGroupHandlesKHR between memory map/unmap
    std::vector<uint8_t> temp_buffer_aligned(shader_binding_table_size_aligned, 0);
    memcpy(temp_buffer_aligned.data(), temp_buffer.data(), raytracing_properties.shaderGroupHandleSize);
    memcpy(temp_buffer_aligned.data() + offset_raygen_side_group, temp_buffer.data() + raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleSize);
    memcpy(temp_buffer_aligned.data() + offset_miss_group, temp_buffer.data() + 2 * raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleSize);
    memcpy(temp_buffer_aligned.data() + offset_hit_group, temp_buffer.data() + 3 * raytracing_properties.shaderGroupHandleSize, 2 * raytracing_properties.shaderGroupHandleSize);

    shader_binding_table = Allocated_buffer(
        vk::BufferCreateInfo{
            .size = shader_binding_table_size_aligned,
            .usage = vk::BufferUsageFlagBits::eRayTracingKHR
        },
        temp_buffer_aligned.data(),
        context.device, context.allocator, context.command_pool, context.graphics_queue);
}

void Raytracing_pipeline::create_pipeline(Context& context, Scene& scene)
{
    std::vector shader_stages{
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eRaygenKHR)
            .setModule(scene.raygen_center_shader.shader_module)
            .setPName("main"),
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eRaygenKHR)
            .setModule(scene.raygen_side_shader.shader_module)
            .setPName("main"),
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eMissKHR)
            .setModule(scene.miss_shader.shader_module)
            .setPName("main")
    };
    std::vector groups{
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(0) // Raygen shader id
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(1) // Raygen shader id
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(2) // miss shader id
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR)
    };

    shader_stages.reserve(shader_stages.size() + 2 * scene.entities.size());
    groups.reserve(shader_stages.size() + scene.entities.size());
    uint32_t id = 3;
    for (const auto shader_group : scene.entities) {
        shader_stages.push_back(vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eIntersectionKHR)
            .setModule(shader_group.intersection.shader_module)
            .setPName("main"));
        shader_stages.push_back(vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
            .setModule(shader_group.closest_hit.shader_module)
            .setPName("main"));
        groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup)
            .setIntersectionShader(id) // intersection shader id
            .setClosestHitShader(id + 1) // closest hit shader id
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR));
        id += 2;
    }

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
}

void Raytracing_pipeline::reload(Context& /*context*/)
{
    //m_device.destroyPipeline(pipeline);
    //create_pipeline(context);
}

}