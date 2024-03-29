#pragma once

#include "vr_common.hpp"
#include "core/scene.hpp"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sdf_editor::vr
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

inline void to_glm(xr::Posef pose, Entity& entity) {
    entity.local_transform.position.x = pose.position.x;
    entity.local_transform.position.y = pose.position.y;
    entity.local_transform.position.z = pose.position.z;
    entity.local_transform.rotation.w = pose.orientation.w;
    entity.local_transform.rotation.x = pose.orientation.x;
    entity.local_transform.rotation.y = pose.orientation.y;
    entity.local_transform.rotation.z = pose.orientation.z;
    entity.dirty_global = true;
}

}