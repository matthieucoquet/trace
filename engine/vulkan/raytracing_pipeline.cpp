#include "raytracing_pipeline.hpp"
#include "context.hpp"
#include "core/scene.hpp"
#include "command_buffer.hpp"
#include <fstream>

namespace vulkan
{

Raytracing_pipeline::Raytracing_pipeline(Context& context, Scene& scene, vk::Sampler immutable_sampler) :
    m_device(context.device)
{
    vk::PhysicalDeviceProperties2 properties{};
    properties.pNext = &raytracing_properties;
    context.physical_device.getProperties2(&properties);

    std::array array_bindings
    {
        vk::DescriptorSetLayoutBinding{
            .binding = 0u,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR },
        vk::DescriptorSetLayoutBinding{  // Output image
            .binding = 1u,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR },
        vk::DescriptorSetLayoutBinding{  // Noise texture
            .binding = 2u,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eAnyHitKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR ,
            .pImmutableSamplers = &immutable_sampler },
        vk::DescriptorSetLayoutBinding{  // Objects transform
            .binding = 3u,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eAnyHitKHR | vk::ShaderStageFlagBits::eClosestHitKHR },
        vk::DescriptorSetLayoutBinding{  // Materials
            .binding = 4u,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR },
        vk::DescriptorSetLayoutBinding{  // Lights
            .binding = 5u,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR }
    };
    descriptor_set_layout = m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
        .bindingCount = static_cast<uint32_t>(array_bindings.size()),
        .pBindings = array_bindings.data() });

    vk::PushConstantRange push_constants{
        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | 
            vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eAnyHitKHR |
            vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR,
        .offset = 0,
        .size = sizeof(Scene_global) };

    pipeline_layout = m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo{
        .setLayoutCount = 1u,
        .pSetLayouts = &descriptor_set_layout,
        .pushConstantRangeCount = 1u,
        .pPushConstantRanges = &push_constants });

    create_pipeline(scene);

    auto temp_buffer_aligned = create_shader_binding_table();
    One_time_command_buffer command_buffer(context.device, context.command_pool, context.graphics_queue);
    Buffer_from_staged buffer_and_staged(
        context.device, context.allocator, command_buffer.command_buffer,
        vk::BufferCreateInfo{
            .size = temp_buffer_aligned.size(),
            .usage = vk::BufferUsageFlagBits::eRayTracingKHR
        },
        temp_buffer_aligned.data());
    shader_binding_table = std::move(buffer_and_staged.result);
    command_buffer.submit_and_wait_idle();
}

Raytracing_pipeline::~Raytracing_pipeline()
{
    m_device.destroyPipeline(pipeline);
    m_device.destroyPipelineLayout(pipeline_layout);
    m_device.destroyDescriptorSetLayout(descriptor_set_layout);
}

std::vector<uint8_t> Raytracing_pipeline::create_shader_binding_table()
{
    auto group_count = static_cast<uint32_t>(2 + nb_group_miss + 2 * nb_group_primary);

    auto find_alignement = [alignement = raytracing_properties.shaderGroupBaseAlignment](vk::DeviceSize offset) {
        auto ret = offset % alignement;
        return ret == 0 ? offset : offset + alignement - ret;
    };

    offset_raygen_side_group = find_alignement(1u * raytracing_properties.shaderGroupHandleSize);
    offset_miss_group = find_alignement(offset_raygen_side_group + 1u * raytracing_properties.shaderGroupHandleSize);
    offset_hit_group = find_alignement(offset_miss_group + nb_group_miss * raytracing_properties.shaderGroupHandleSize);

    auto shader_binding_table_size = raytracing_properties.shaderGroupHandleSize * group_count;
    auto shader_binding_table_size_aligned = static_cast<uint32_t>(offset_hit_group + raytracing_properties.shaderGroupHandleSize * 2 * nb_group_primary);

    std::vector<uint8_t> temp_buffer = m_device.getRayTracingShaderGroupHandlesKHR<uint8_t>(pipeline, 0u, group_count, shader_binding_table_size);

    std::vector<uint8_t> temp_buffer_aligned(shader_binding_table_size_aligned, 0);
    // Copy raygen narrow
    memcpy(temp_buffer_aligned.data(), temp_buffer.data(), raytracing_properties.shaderGroupHandleSize);
    // Copy raygen wide
    memcpy(temp_buffer_aligned.data() + offset_raygen_side_group, temp_buffer.data() + raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleSize);
    // Copy miss
    memcpy(temp_buffer_aligned.data() + offset_miss_group, temp_buffer.data() + 2 * raytracing_properties.shaderGroupHandleSize, nb_group_miss * raytracing_properties.shaderGroupHandleSize);
    // Copy hit
    memcpy(temp_buffer_aligned.data() + offset_hit_group, temp_buffer.data() + (2 + nb_group_miss) * raytracing_properties.shaderGroupHandleSize, (2 * nb_group_primary) * raytracing_properties.shaderGroupHandleSize);
    return temp_buffer_aligned;
}

void Raytracing_pipeline::create_pipeline(Scene& scene)
{
    std::vector shader_stages{
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
            .module = scene.raygen_narrow_shader.module,
            .pName = "main" },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
            .module = scene.raygen_wide_shader.module,
            .pName = "main" },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eMissKHR,
            .module = scene.primary_miss_shader.module,
            .pName = "main" },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eMissKHR,
            .module = scene.shadow_miss_shader.module,
            .pName = "main" },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eIntersectionKHR,
            .module = scene.shadow_intersection_shader.module,
            .pName = "main" }
    };
    std::vector groups{
        // Raygens (0 and 1)
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 0, // Raygen shader id
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR },
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 1, // Raygen shader id
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR },
        // Miss (2 and 3)
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 2, // miss shader id
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR},
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 3, // miss shader id
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR},
    };
    nb_group_miss = 2u;
    nb_group_primary = scene.shader_groups.size();

    shader_stages.reserve(shader_stages.size() + 3 * scene.shader_groups.size());
    groups.reserve(groups.size() + 2 * nb_group_primary);
    uint32_t id = static_cast<uint32_t>(shader_stages.size());
    // 4 -> 3 + 2 * m_nb_group_primary
    for (const auto shader_group : scene.shader_groups) {
        shader_stages.push_back(vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eIntersectionKHR,
            .module = shader_group.primary_intersection.module,
            .pName = "main" });
        shader_stages.push_back(vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eClosestHitKHR,
            .module = shader_group.primary_closest_hit.module,
            .pName = "main" });
        shader_stages.push_back(vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eAnyHitKHR,
            .module = shader_group.shadow_any_hit.module,
            .pName = "main" });

        groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR{  // Primary
            .type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = id + 1,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = id });
        groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR{  // Shadow
            .type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = id + 2,
            .intersectionShader = 4 });
        id += 3;
    }

    pipeline = m_device.createRayTracingPipelineKHR(
        nullptr,
        vk::RayTracingPipelineCreateInfoKHR{
            //.flags = vk::PipelineCreateFlagBits::eRayTracingSkipTrianglesKHR,
            .stageCount = static_cast<uint32_t>(shader_stages.size()),
            .pStages = shader_stages.data(),
            .groupCount = static_cast<uint32_t>(groups.size()),
            .pGroups = groups.data(),
            .maxRecursionDepth = std::max(3u, raytracing_properties.maxRecursionDepth),
            .layout = pipeline_layout }).value;
}

}