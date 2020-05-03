#include "scene.h"
#include "context.h"

Scene::Scene(Context& context)
{
    primitives.emplace_back(Primitive{ .center = glm::vec3(2.0f, -1.0f, 0.0f) });
    primitives.emplace_back(Primitive{ .center = glm::vec3(-2.0f, -1.0f, 0.0f) });
    primitives.emplace_back(Primitive{ .center = glm::vec3(2.0f, 1.0f, 0.0f) });
    primitives.emplace_back(Primitive{ .center = glm::vec3(-2.0f, 1.0f, 0.0f) });
    kinds.emplace_back(Object_kind::cube);
    kinds.emplace_back(Object_kind::sphere);
    kinds.emplace_back(Object_kind::sphere);
    kinds.emplace_back(Object_kind::cube);

    aabbs.emplace_back(- 1.0f, - 1.0f, - 1.0f, 1.0f, 1.0f, 1.0f);
    aabbs_buffer = Allocated_buffer(
        vk::BufferCreateInfo()
            .setSize(sizeof(Aabb) * aabbs.size())
            .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eRayTracingKHR),  // No other usage ?
        aabbs.data(),
        context.device, context.allocator, context.command_pool, context.graphics_queue);

    primitives_buffer = Allocated_buffer(
        vk::BufferCreateInfo()
        .setSize(sizeof(Primitive) * primitives.size())
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eRayTracingKHR),  // No other usage ?
        primitives.data(),
        context.device, context.allocator, context.command_pool, context.graphics_queue);

}