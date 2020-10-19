#include "ingenuity.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ingenuity
{

Scene make_scene()
{
    Scene scene{};
    scene.shader_groups.push_back(Shader_group{ .name = "hand" });
    scene.shader_groups.push_back(Shader_group{ .name = "ingenuity" });
    size_t hand_id = 0u;
    size_t ingenuity_id = 1u;

    for (unsigned int i = 0u; i < 2u; i++) {
        scene.objects.emplace_back(Object{
            .name = i == 0 ? "left_hand" : "right_hand",
            .position = glm::vec3(0.0f, 1.0f, 0.0f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.12f,
            .group_id = hand_id });
    }


    scene.objects.emplace_back(Object{
        .name = std::string("ingenuity"),
        .position = glm::vec3(-1.5f, 1.0f, -1.5f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.5f,
        .group_id = ingenuity_id });
 
    for (auto& object : scene.objects) {
        glm::mat4 model_to_world = glm::translate(object.position) * glm::toMat4(object.rotation) * glm::scale(glm::vec3(object.scale));
        scene.objects_transform.emplace_back(glm::inverse(model_to_world));
    }

    scene.ui_object = Object{ .position = glm::vec3(0.f, 1.5f, -0.5f), .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), .scale = 0.4f };
    scene.scene_global.ui_position = scene.ui_object.position;
    scene.scene_global.ui_normal = glm::rotate(scene.ui_object.rotation, glm::vec3(0.0f, 0.0f, 1.0f));

    scene.materials.push_back(Material{ .color = glm::vec3(0.05f) }); // White
    scene.materials.push_back(Material{ .color = glm::vec3(0.8f) }); // Black
    scene.materials.push_back(Material{ .color = glm::vec3(0.95f, 0.05f, 0.15f) }); // Red
    scene.materials.push_back(Material{ .color = glm::vec3(0.765f, 0.494f, 0.341f) }); // Rock
    scene.materials.push_back(Material{ .color = glm::vec3(0.424f, 0.298f, 0.231f) }); // Sand

    scene.lights.push_back(Light{ .position = glm::vec3{ 20.0f, 60.0f, -100.0f }, .color = glm::vec3{ 1.0f } });
    //scene.lights.push_back(Light{ .position = glm::vec3{ -20.0f, 20.0f, 20.0f }, .color = glm::vec3{ 1.0f } });
    return scene;
}

Ingenuity::Ingenuity() :
    Engine(make_scene(), SHADER_SOURCE)
{
}

}