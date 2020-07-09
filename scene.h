#pragma once
#include <vector>
#include <chrono>
#include "shader_types.h"
#include "entities.h"
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

    Scene_global scene_global = {};

    vulkan::Allocated_buffer aabbs_buffer;
    vulkan::Allocated_buffer primitives_buffer;

    std::vector<Shader_file> shader_files;
    Shader raygen_shader;
    Shader miss_shader;
    std::vector<Shader_group> entities;

    Scene(vulkan::Context& context);

    void step();
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float>;

    Time_point m_start_clock = Clock::now();
};