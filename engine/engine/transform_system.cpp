#include "transform_system.hpp"
#include "core/scene.hpp"

namespace sdf_editor
{

static size_t update_entity(Scene& scene, Entity& entity, size_t id = 0, const Entity* parent = nullptr)
{
    if (entity.dirty_local) {
        if (parent) {
            entity.global_transform.scale = parent->global_transform.scale * entity.local_transform.scale;
            entity.local_transform = parent->global_transform.inverse() * entity.global_transform;
        }
        else {
            entity.global_transform.scale = entity.local_transform.scale;
            entity.local_transform = entity.global_transform;
        }
        for (auto& child : entity.children) {
            child.dirty_global = true;
        }
        entity.dirty_local = false;
    }
    if (entity.dirty_global) {
        if (parent) {
            entity.global_transform = parent->global_transform * entity.local_transform;
        }
        else {
            entity.global_transform = entity.local_transform;
        }
        for (auto& child : entity.children) {
            child.dirty_global = true;
        }
        entity.dirty_global = false;
    }

    glm::mat4 inv = glm::translate(entity.global_transform.position) * glm::toMat4(entity.global_transform.rotation) * glm::scale(glm::vec3(entity.global_transform.scale));
    if (scene.entities_instances.size() > id) {
        scene.entities_instances[id].transform.matrix = std::array<std::array<float, 4>, 3>{
            std::array<float, 4>{ inv[0].x, inv[1].x, inv[2].x, inv[3].x },
                std::array<float, 4>{ inv[0].y, inv[1].y, inv[2].y, inv[3].y },
                std::array<float, 4>{ inv[0].z, inv[1].z, inv[2].z, inv[3].z }
        };
    }
    else {
        scene.entities_instances.push_back(vk::AccelerationStructureInstanceKHR{
            .transform = {
                .matrix = std::array<std::array<float, 4>, 3>{
                    std::array<float, 4>{ inv[0].x, inv[1].x, inv[2].x, inv[3].x },
                    std::array<float, 4>{ inv[0].y, inv[1].y, inv[2].y, inv[3].y },
                    std::array<float, 4>{ inv[0].z, inv[1].z, inv[2].z, inv[3].z }
            } },
            .instanceCustomIndex = static_cast<uint32_t>(id),
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 2 * static_cast<uint32_t>(entity.group_id), // 2 for primary + shadow
            });
    }

    for (auto& child : entity.children) {
        id = update_entity(scene, child, id + 1, &entity);
    }
    return id;
}

Transform_system::Transform_system(Scene& scene)
{
    for (auto& entity : scene.root.entities) {
        entity.dirty_global = true;
    }
    step(scene);
}

void Transform_system::step(Scene& scene)
{
    size_t id = 0;
    for (auto& entity : scene.root.entities)
    {
        id = update_entity(scene, entity, id);
        id++;
    }
}

}