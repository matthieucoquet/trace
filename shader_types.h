#pragma once
#include "common.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <compare>

using Aabb = vk::AabbPositionsKHR;

enum class Object_kind : unsigned int
{
    sphere = 0u,
    cube = 1u
};

struct Primitive
{
    glm::vec3 center;
};
