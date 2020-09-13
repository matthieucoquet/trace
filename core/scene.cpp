
#include "scene.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene()
{
    shader_groups.push_back(Shader_group{ .name = "cube" });
    shader_groups.push_back(Shader_group{ .name = "sphere"});
    shader_groups.push_back(Shader_group{ .name = "hand" });
    size_t sphere_id = 1u;
    size_t cube_id = 0u;
    size_t hand_id = 2u;

    for (unsigned int i = 0u; i < 2u; i++) {
        objects.emplace_back(Object{ 
            .name = i == 0 ? "left_hand" : "right_hand",
            .position = glm::vec3(0.0f, 1.0f, 0.0f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.1f,
            .group_id = hand_id });
    }

    for (unsigned int i = 0u; i < 4u; i++) {
        for (unsigned int j = 0u; j < 4u; j++) {
            objects.emplace_back(Object{
                .name = std::string("sphere_") + std::to_string(4 * i + j),
                .position = glm::vec3(-1.5f + 1.0f * i, 1.5f, -1.5f + 1.0f * j), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.5f,
                .group_id = sphere_id });
        }
    }

    for (unsigned int i = 0u; i < 4u; i++) {
        for (unsigned int j = 0u; j < 4u; j++) {
            objects.emplace_back(Object{
                .name = std::string("cube_") + std::to_string(4 * i + j),
                .position = glm::vec3(-3.0f + 2.0f * i, 3.0f, -3.0f + 2.0f * j), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 1.0f,
                .group_id = cube_id });
        }
    }

    for (auto& object : objects) {
        glm::mat4 model_to_world = glm::translate(object.position) * glm::toMat4(object.rotation) * glm::scale(glm::vec3(object.scale));
        objects_transform.emplace_back(glm::inverse(model_to_world));
    }

    ui_object = Object{ .position = glm::vec3(0.f, 1.5f - vr_offset_y, -0.5f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.4f };
    scene_global.ui_position = ui_object.position;
    scene_global.ui_normal = glm::rotate(ui_object.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
}

void Scene::step()
{
    Duration time_since_start = Clock::now() - m_start_clock;
    scene_global.time = time_since_start.count();
}