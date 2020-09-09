
#include "scene.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene()
{
    shader_groups.push_back(Shader_group{ .name = "sphere"});
    shader_groups.push_back(Shader_group{ .name = "cube" });
    shader_groups.push_back(Shader_group{ .name = "hand" });
    size_t sphere_id = 0u;
    size_t cube_id = 1u;
    size_t hand_id = 2u;

    for (unsigned int i = 0u; i < 2u; i++) {
        primitives.emplace_back(Primitive{ .position = glm::vec3(0.0f, 1.0f, 0.0f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.1f });
        primitive_group_ids.emplace_back(hand_id);
    }

    for (unsigned int i = 0u; i < 4u; i++) {
        for (unsigned int j = 0u; j < 4u; j++) {
            primitives.emplace_back(Primitive{ .position = glm::vec3(-1.5f + 1.0f * i, 1.5f, -1.5f + 1.0f * j), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.5f });
            primitive_group_ids.emplace_back(sphere_id);
        }
    }

    for (unsigned int i = 0u; i < 4u; i++) {
        for (unsigned int j = 0u; j < 4u; j++) {
            primitives.emplace_back(Primitive{ .position = glm::vec3(-3.0f + 2.0f * i, 3.0f, -3.0f + 2.0f * j), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 1.0f });
            primitive_group_ids.emplace_back(cube_id);
        }
    }

    for (auto& primitive : primitives) {
        glm::mat4 model_to_world = glm::translate(primitive.position) * glm::toMat4(primitive.rotation) * glm::scale(glm::vec3(primitive.scale));
        primitive_transform.emplace_back(glm::inverse(model_to_world));
    }

    ui_primitive = Primitive{ .position = glm::vec3(0.f, 1.5f - vr_offset_y, -0.5f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.4f };
    scene_global.ui_position = ui_primitive.position;
    scene_global.ui_normal = glm::rotate(ui_primitive.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
}

void Scene::step()
{
    Duration time_since_start = Clock::now() - m_start_clock;
    scene_global.time = time_since_start.count();
}