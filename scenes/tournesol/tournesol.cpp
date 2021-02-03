#include "tournesol.hpp"
#include <limits>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace tournesol
{

using namespace sdf_editor;

Scene make_scene()
{
    Scene scene{};
    scene.shaders.groups.push_back(Shader_group{ .name = "hand" });
    scene.shaders.groups.push_back(Shader_group{ .name = "ui" });
    scene.shaders.groups.push_back(Shader_group{ .name = "rocket" });

    size_t hand_id = 0u;
    size_t ui_id = 1u;
    size_t rocket_id = 2u;
    size_t no_shader_id = std::numeric_limits<size_t>::max();

    for (unsigned int i = 0u; i < 2u; i++) {
        scene.entities.emplace_back(Entity{
            .name = i == 0 ? "left_hand" : "right_hand",
            .local_transform = Transform{
                .position = glm::vec3(0.0f, 1.0f, 0.0f),
                .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                .scale = 0.12f
            },
            .group_id = hand_id });
    }

    scene.entities.emplace_back(Entity{
            .name = "ui",
            .local_transform = Transform{
                .position = glm::vec3(0.0f, 1.8f, -0.5f),
                .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                .scale = 0.7f
            },
            .group_id = ui_id });

    scene.entities.emplace_back(Entity{
        .name = "tournesol",
        .group_id = std::numeric_limits<size_t>::max()
        });

    return scene;
}

Tournesol::Tournesol() :
    App(make_scene(), SCENE_JSON, SHADER_SOURCE)
{
}

}