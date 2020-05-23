#pragma once
#include <vector>
#include "shader_types.h"
#include "vulkan/allocation.h"

namespace vulkan
{
class Context;
}

class Scene
{
public:
    std::vector<Aabb> aabbs;
    std::vector<Primitive> primitives;
    std::vector<Object_kind> kinds;

    vulkan::Allocated_buffer aabbs_buffer;
    vulkan::Allocated_buffer primitives_buffer;

    Scene(vulkan::Context& context);

private:

};