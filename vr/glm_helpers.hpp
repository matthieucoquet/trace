#pragma once

#include "vr_common.hpp"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vr
{
inline xr::Vector3f to_xr(glm::vec3 vec) {
    return { vec.x, vec.y, vec.z };
}

inline xr::Quaternionf to_xr(glm::quat quat) {
    return { .x = quat.x, .y = quat.y, .z = quat.z, .w = quat.w };
}

inline xr::Posef to_xr(glm::vec3 vec, glm::quat quat) {
    return { .orientation = to_xr(quat), .position = to_xr(vec) };
}

}