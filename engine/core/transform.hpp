#pragma once
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace sdf_editor
{

struct Transform
{
    glm::vec3 position{};
    glm::quat rotation{1.0, 0.0, 0.0, 0.0};
    float scale{ 1.0f };
    glm::vec3 flip_axis{ 1.0 };

    Transform operator*(const Transform& rhs) const
    {
        Transform result = *this;
        result *= rhs;
        return result;
    }
    Transform& operator*=(const Transform& rhs)
    {
        position += glm::rotate(rotation, scale * rhs.position);
        rotation = rotation * rhs.rotation;
        scale = scale * rhs.scale;
        flip_axis = rhs.flip_axis;
        return *this;
    }

    Transform inverse() const {
        glm::quat conj = glm::conjugate(rotation);
        return Transform{
            .position = glm::rotate(conj, -position / scale),
            .rotation = conj,
            .scale = 1.0f / scale
        };
    }

};

}