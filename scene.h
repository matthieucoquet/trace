#pragma once
#include <vector>
#include "shader_types.h"
#include "allocation.h"

class Context;

class Scene
{
public:
    std::vector<Aabb> aabbs;
    std::vector<Primitive> primitives;
    std::vector<Object_kind> kinds;

    Allocated_buffer aabbs_buffer;
    Allocated_buffer primitives_buffer;

    Scene(Context& context);

private:

};