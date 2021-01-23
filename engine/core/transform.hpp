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

    Transform inverse() const {
        return Transform{
            .position = - glm::inverse(rotation) * position,
            .rotation = glm::inverse(rotation)
        };
    }

};

}