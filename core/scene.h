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
    Shader raygen_side_shader;
    Shader raygen_center_shader;
    Shader miss_shader;
    std::vector<Shader_group> entities;

    std::array<xr::Posef, 2> last_known_hand_pose;

    Scene(vulkan::Context& context);

    void step();
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float>;

    Time_point m_start_clock = Clock::now();
};