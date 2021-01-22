#pragma once
#include "vulkan/vk_common.hpp"
#include "vr/vr_common.hpp"
#include <array>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sdf_editor
{

struct Eye
{
    xr::Posef pose = {};
    xr::Fovf fov = {};
};

struct Scene_global
{
    std::array<Eye, 2> eyes;
    glm::vec3 ui_position;
    glm::vec3 ui_normal;
    float time = {};
    int nb_lights = {};
};

struct Material
{
    glm::vec3 color;
};

struct Light
{
    glm::vec3 position;
    glm::vec3 color;
};

}
