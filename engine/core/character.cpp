#include "character.hpp"

namespace sdf_editor
{

static vk::AabbPositionsKHR aabb(glm::vec3 min, glm::vec3 max)
{
    return vk::AabbPositionsKHR{
        .minX = min.x, .minY = min.y, .minZ = min.z,
        .maxX = max.x, .maxY = max.y, .maxZ = max.z
    };
}

void Character::update_global()
{
    for (Joint& joint : joints)
    {
        if (joint.parent) {
            joint.global = joint.parent->global * joint.local;
        }
        else {
            joint.global = joint.local;
        }
        joint.body.global = joint.global * joint.body.local;
    }

    aabbs.resize(joints.size());
    for (size_t i = 0u; i < aabbs.size(); i++)
    {
        glm::vec3 pos = joints[i].body.global.position;
        aabbs[i] = aabb(pos - 0.5f, pos + 0.5f);
    }
}

}