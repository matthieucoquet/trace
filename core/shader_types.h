#pragma once
#include "vulkan/vk_common.h"
#include "vr/vr_common.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

using Aabb = vk::AabbPositionsKHR;

enum class Object_kind : unsigned int
{
    sphere = 0u,
    cube = 1u
};

struct Primitive
{
    glm::vec3 center;
    float padding;
};

// TODO use push constant
struct Eye
{
    xr::Posef pose = {};
    float padding = {};
    xr::Fovf fov = {};
};

struct Scene_global
{
    std::array<Eye, 2> eyes;
    float time = {};
};
