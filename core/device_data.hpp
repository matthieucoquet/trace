#pragma once
#include "vulkan/vk_common.hpp"
#include "vr/vr_common.hpp"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

struct Primitive
{
    glm::mat4 world_to_model;
};

struct Eye
{
    xr::Posef pose = {};
    xr::Fovf fov = {};
};

struct Scene_global
{
    std::array<Eye, 2> eyes;
    float time = {};
};
