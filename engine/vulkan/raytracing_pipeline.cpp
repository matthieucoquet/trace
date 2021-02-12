#include "raytracing_pipeline.hpp"
#include "context.hpp"
#include "core/scene.hpp"
#include "command_buffer.hpp"
#include <fstream>
#include <fmt/core.h>

namespace sdf_editor::vulkan
{

Raytracing_pipeline::Raytracing_pipeline(Context& context, Scene& scene, vk::Sampler immutable_sampler_noise, vk::Sampler immutable_sampler_ui) :
    m_device(context.device)
{
    vk::PhysicalDeviceProperties2 properties{};
    properties.pNext = &raytracing_properties;
    context.physical_device.getProperties2(&properties);
    fmt::print("Max ray {}\n", raytracing_properties.maxRayDispatchInvocationCount);

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
        vk::DescriptorSetLayoutBinding{  // Materials
            .binding = 2u,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR },
        vk::DescriptorSetLayoutBinding{  // Lights
            .binding = 3u,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR },
        vk::DescriptorSetLayoutBinding{  // Noise texture TODO merge all texture to an array
            .binding = 4u,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eAnyHitKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR ,
            .pImmutableSamplers = &immutable_sampler_noise },
        vk::DescriptorSetLayoutBinding{  // UI texture
            .binding = 5u,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = &immutable_sampler_ui },
        vk::DescriptorSetLayoutBinding{  // Scene texture
            .binding = 6u,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eMissKHR,
            .pImmutableSamplers = &immutable_sampler_noise }
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
            .usage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress
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
    auto group_count = static_cast<uint32_t>(1 + nb_group_miss + 3 * nb_group_primary);

    auto base_alignement = [alignement = raytracing_properties.shaderGroupBaseAlignment](vk::DeviceSize offset) {
        auto ret = offset % alignement;
        return ret == 0 ? offset : offset + alignement - ret;
    };
    uint32_t handle_size = raytracing_properties.shaderGroupHandleSize;
    if (handle_size % raytracing_properties.shaderGroupHandleAlignment != 0) {
        handle_size = handle_size - handle_size % raytracing_properties.shaderGroupHandleAlignment - raytracing_properties.shaderGroupHandleAlignment;
    }
    offset_miss_group = base_alignement(1u * handle_size);
    offset_hit_group = base_alignement(offset_miss_group + nb_group_miss * handle_size);

    auto shader_binding_table_size = raytracing_properties.shaderGroupHandleSize * group_count;
    auto shader_binding_table_size_aligned = static_cast<uint32_t>(offset_hit_group + handle_size * 3 * nb_group_primary);

    std::vector<uint8_t> temp_buffer = m_device.getRayTracingShaderGroupHandlesKHR<uint8_t>(pipeline, 0u, group_count, shader_binding_table_size);

    std::vector<uint8_t> temp_buffer_aligned(shader_binding_table_size_aligned, 0);
    // Copy raygen
    memcpy(temp_buffer_aligned.data(), temp_buffer.data(), raytracing_properties.shaderGroupHandleSize);
    // Copy miss
    memcpy(temp_buffer_aligned.data() + offset_miss_group, temp_buffer.data() + 1 * raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleSize);
    memcpy(temp_buffer_aligned.data() + offset_miss_group + handle_size, temp_buffer.data() + 2 * raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleSize);
    // Copy hit
    for (int i = 0; i < nb_group_primary; i++) {
        memcpy(temp_buffer_aligned.data() + offset_hit_group + 3 * i * handle_size, temp_buffer.data() + (1 + nb_group_miss + 3 * i) * raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleSize);
        memcpy(temp_buffer_aligned.data() + offset_hit_group + (3 * i + 1) * handle_size, temp_buffer.data() + (1 + nb_group_miss + 3 * i + 1) * raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleSize);
        memcpy(temp_buffer_aligned.data() + offset_hit_group + (3 * i + 2) * handle_size, temp_buffer.data() + (1 + nb_group_miss + 3 * i + 2) * raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleSize);
    }
    shader_binding_table_stride = handle_size;
    return temp_buffer_aligned;
}

void Raytracing_pipeline::create_pipeline(Scene& scene)
{
    std::vector shader_stages{
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
            .module = scene.shaders.raygen.module,
            .pName = "main" },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eMissKHR,
            .module = scene.shaders.primary_miss.module,
            .pName = "main" },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eMissKHR,
            .module = scene.shaders.shadow_miss.module,
            .pName = "main" },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eIntersectionKHR,
            .module = scene.shaders.shadow_intersection.module,
            .pName = "main" }
    };
    std::vector groups{
        // Raygens (0)
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 0, // Raygen shader id
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR },
        // Miss (1 and 2)
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 1, // miss shader id
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR},
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 2, // miss shader id
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR},
    };
    nb_group_miss = 2u;
    nb_group_primary = scene.shaders.groups.size();

    shader_stages.reserve(shader_stages.size() + 4 * nb_group_primary);
    groups.reserve(groups.size() + 3 * nb_group_primary);
    uint32_t id = static_cast<uint32_t>(shader_stages.size());

    for (const auto shader_group : scene.shaders.groups) {
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
        shader_stages.push_back(vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eAnyHitKHR,
            .module = shader_group.ao_any_hit.module,
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
            .intersectionShader = 3 });
        groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR{  // AO
            .type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = id + 3,
            .intersectionShader = 3 });
        id += 4;
    }

    pipeline = m_device.createRayTracingPipelineKHR(
        nullptr, nullptr,
        vk::RayTracingPipelineCreateInfoKHR{
            //.flags = vk::PipelineCreateFlagBits::eRayTracingSkipTrianglesKHR,
            .stageCount = static_cast<uint32_t>(shader_stages.size()),
            .pStages = shader_stages.data(),
            .groupCount = static_cast<uint32_t>(groups.size()),
            .pGroups = groups.data(),
            .maxPipelineRayRecursionDepth = 4, //raytracing_properties.maxRayRecursionDepth, // * std::min(4u, raytracing_properties.maxRayRecursionDepth),
            .layout = pipeline_layout }).value;
}

}