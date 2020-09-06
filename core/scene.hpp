#pragma once
#include "device_data.hpp"
#include "shader_data.hpp"
#include <vector>
#include <chrono>

struct Primitive
{
    glm::vec3 position;
    glm::quat rotation;
    float scale;
};

class Scene
{
public:
    // Should probably be a runtime setting in the future
    static constexpr bool standing = true;
    static constexpr float vr_offset_y = standing ? 0.0f : 1.7f;

    Scene_global scene_global = {};

    std::vector<Primitive> primitives;
    std::vector<glm::mat4> primitive_transform;
    std::vector<size_t> primitive_group_ids;

    Primitive ui_primitive;

    std::vector<Shader_file> shader_files;
    Shader raygen_side_shader;
    Shader raygen_center_shader;
    Shader miss_shader;
    std::vector<Shader_group> shader_groups;

    Scene();

    void step();
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float>;

    Time_point m_start_clock = Clock::now();
};