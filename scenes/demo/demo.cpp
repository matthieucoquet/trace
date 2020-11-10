#include "demo.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace demo
{

Scene make_scene()
{
    Scene scene{};
    scene.shader_groups.push_back(Shader_group{ .name = "cube" });
    scene.shader_groups.push_back(Shader_group{ .name = "sphere" });
    scene.shader_groups.push_back(Shader_group{ .name = "hand" });
    size_t sphere_id = 1u;
    size_t cube_id = 0u;
    size_t hand_id = 2u;

    for (unsigned int i = 0u; i < 2u; i++) {
        scene.objects.emplace_back(Object{
            .name = i == 0 ? "left_hand" : "right_hand",
            .position = glm::vec3(0.0f, 1.0f, 0.0f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.12f,
            .group_id = hand_id });
    }

    for (unsigned int i = 0u; i < 4u; i++) {
        for (unsigned int j = 0u; j < 4u; j++) {
            scene.objects.emplace_back(Object{
                .name = std::string("sphere_") + std::to_string(4 * i + j),
                .position = glm::vec3(-1.5f + 1.0f * i, 1.5f, -1.5f + 1.0f * j), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.5f,
                .group_id = sphere_id });
        }
    }

    for (unsigned int i = 0u; i < 4u; i++) {
        for (unsigned int j = 0u; j < 4u; j++) {
            scene.objects.emplace_back(Object{
                .name = std::string("cube_") + std::to_string(4 * i + j),
                .position = glm::vec3(-3.0f + 2.0f * i, 3.0f, -3.0f + 2.0f * j), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 1.0f,
                .group_id = cube_id });
        }
    }

    for (auto& object : scene.objects) {
        glm::mat4 model_to_world = glm::translate(object.position) * glm::toMat4(object.rotation) * glm::scale(glm::vec3(object.scale));
        scene.objects_transform.emplace_back(glm::inverse(model_to_world));
    }

    scene.ui_object = Object{ .position = glm::vec3(0.f, 1.5f, -0.5f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.4f };
    scene.scene_global.ui_position = scene.ui_object.position;
    scene.scene_global.ui_normal = glm::rotate(scene.ui_object.rotation, glm::vec3(0.0f, 0.0f, 1.0f));

    scene.materials.push_back(Material{ .color = glm::vec3(0.05f) });
    scene.materials.push_back(Material{ .color = glm::vec3(0.8f) });
    scene.materials.push_back(Material{ .color = glm::vec3(0.95f, 0.05f, 0.15f) });
    scene.materials.push_back(Material{ .color = glm::vec3(0.1f, 0.15f, 0.85f) });

    scene.lights.push_back(Light{ .position = glm::vec3{ 20.0f, 60.0f, -100.0f }, .color = glm::vec3{ 1.0f } });
    return scene;
}

Demo::Demo() :
    App(make_scene(), SHADER_SOURCE)
{
}

}