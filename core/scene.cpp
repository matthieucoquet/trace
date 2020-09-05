
#include "scene.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

Scene::Scene()
{
    shader_groups.push_back(Shader_group{ .name = "sphere"});
    shader_groups.push_back(Shader_group{ .name = "cube" });
    size_t sphere_id = 0u;
    size_t cube_id = 1u;

    for (unsigned int i = 0u; i < 2u; i++) {
        glm::mat4 model_to_world = glm::translate(glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(0.1f));
        primitives.emplace_back(Primitive{ .world_to_model = glm::inverse(model_to_world) });
        primitive_group_ids.emplace_back(sphere_id);
    }

    for (unsigned int i = 0u; i < 4u; i++) {
        for (unsigned int j = 0u; j < 4u; j++) {
            glm::mat4 model_to_world = glm::translate(glm::vec3(-6.0f + 4.0f * i, 2.0f, -6.0f + 4.0f * j)) * glm::scale(glm::vec3(0.5f));
            primitives.emplace_back(Primitive{ .world_to_model = glm::inverse(model_to_world) });
            primitive_group_ids.emplace_back(sphere_id);
        }
    }

    for (unsigned int i = 0u; i < 4u; i++) {
        for (unsigned int j = 0u; j < 4u; j++) {
            glm::mat4 model_to_world = glm::translate(glm::vec3(-6.0f + 4.0f * i, 5.0f, -6.0f + 4.0f * j));
            primitives.emplace_back(Primitive{ .world_to_model = glm::inverse(model_to_world) });
            primitive_group_ids.emplace_back(cube_id);
        }
    }
}

void Scene::step()
{
    Duration time_since_start = Clock::now() - m_start_clock;
    scene_global.time = time_since_start.count();

    for (unsigned int i = 0u; i < 2u; i++) {
        glm::mat4 model_to_world = glm::scale(glm::vec3(0.1f));
        model_to_world = glm::toMat4(glm::quat(
            last_known_hand_pose[i].orientation.w,
            last_known_hand_pose[i].orientation.x,
            last_known_hand_pose[i].orientation.y,
            last_known_hand_pose[i].orientation.z)) * model_to_world;
        model_to_world = glm::translate(glm::vec3(last_known_hand_pose[i].position.x, last_known_hand_pose[i].position.y, last_known_hand_pose[i].position.z)) * model_to_world;

        primitives[i].world_to_model = glm::inverse(model_to_world);
    }
}