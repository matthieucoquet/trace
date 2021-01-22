#pragma once
#include "transform_helpers.hpp"
#include "vulkan/vk_common.hpp"
#include <vector>

namespace sdf_editor
{

struct Body
{
    Transform local{};
    Transform global{};
    int id;

};

struct Joint
{
    Transform local{};
    Transform global{};
    Body body;
    Joint* parent = nullptr;
};

class Character
{
public:
    std::vector<Joint> joints;
    std::vector<vk::AabbPositionsKHR> aabbs;
    void update_global();

private:
};

}