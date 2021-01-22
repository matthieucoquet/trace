#pragma once
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace sdf_editor
{

struct Transform
{
    glm::vec3 position;
    glm::quat rotation;

    Transform operator*(const Transform& rhs) const
    {
        Transform result = *this;
        result *= rhs;
        return result;
    }
    Transform& operator*=(const Transform& rhs)
    {
        position += rotation * rhs.position;
        rotation = rotation * rhs.rotation;
        return *this;
    }

};

// TODO clean this
inline Transform transform(glm::vec3 parent_pos, glm::quat parent_rot, glm::vec3 local_child_pos, glm::quat local_child_rot) {
    return Transform{
        parent_pos + parent_rot * local_child_pos,
        parent_rot * local_child_rot
    };
}

inline Transform compute_local_transform(glm::vec3 parent_pos, glm::quat parent_rot, glm::vec3 global_child_pos, glm::quat global_child_rot) {
    return Transform{
        glm::inverse(parent_rot) * (global_child_pos - parent_pos),
        glm::inverse(parent_rot) * global_child_rot
    };
}

}