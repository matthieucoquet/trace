#pragma once
#include "device_data.hpp"
#include "shader_data.hpp"
#include <vector>
#include <chrono>

class Scene
{
public:
    Scene_global scene_global = {};

    std::vector<Primitive> primitives;
    std::vector<size_t> primitive_group_ids;

    std::vector<Shader_file> shader_files;
    Shader raygen_side_shader;
    Shader raygen_center_shader;
    Shader miss_shader;
    std::vector<Shader_group> shader_groups;

    std::array<xr::Posef, 2> last_known_hand_pose;

    Scene();

    void step();
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float>;

    Time_point m_start_clock = Clock::now();
};