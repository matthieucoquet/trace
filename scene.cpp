#include "scene.h"
#include "vulkan/context.h"

Scene::Scene(vulkan::Context& context)
{
    entities.push_back(Shader_group{ .name = "sphere"});
    entities.push_back(Shader_group{ .name = "cube" });

    for (unsigned int i = 0u; i < 8u; i++) {
        for (unsigned int j = 0u; j < 8u; j++) {
            primitives.emplace_back(Primitive{ .center = glm::vec3(-15.0f + 4.0f * i, 2.0f, -15.0f + 4.0f * j) });
            kinds.emplace_back(Object_kind::sphere);
        }
    }

    for (unsigned int i = 0u; i < 8u; i++) {
        for (unsigned int j = 0u; j < 8u; j++) {
            primitives.emplace_back(Primitive{ .center = glm::vec3(-15.0f + 4.0f * i, 5.0f, -15.0f + 4.0f * j) });
            kinds.emplace_back(Object_kind::cube);
        }
    }

    aabbs.emplace_back(- 1.0f, - 1.0f, - 1.0f, 1.0f, 1.0f, 1.0f);
    aabbs_buffer = vulkan::Allocated_buffer(
        vk::BufferCreateInfo()
            .setSize(sizeof(Aabb) * aabbs.size())
            .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eRayTracingKHR),
        aabbs.data(),
        context.device, context.allocator, context.command_pool, context.graphics_queue);

    primitives_buffer = vulkan::Allocated_buffer(
        vk::BufferCreateInfo()
        .setSize(sizeof(Primitive) * primitives.size())
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eRayTracingKHR),
        primitives.data(),
        context.device, context.allocator, context.command_pool, context.graphics_queue);
}

void Scene::step()
{
    Duration time_since_start = Clock::now() - m_start_clock;
    scene_global.time = time_since_start.count();
}