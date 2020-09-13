#pragma once
#include "device_data.hpp"
#include "shader_data.hpp"
#include <vector>
#include <chrono>

struct Object
{
    std::string name;
    glm::vec3 position;
    glm::quat rotation;
    float scale;
    size_t group_id;
};

class Scene
{
public:
    // Should probably be a runtime setting in the future
    static constexpr bool standing = true;
    static constexpr float vr_offset_y = standing ? 0.0f : 1.7f;

    Scene_global scene_global = {};

    std::vector<Object> objects;
    std::vector<glm::mat4> objects_transform;

    Object ui_object;

    bool pipeline_dirty = false;
    std::vector<Shader_file> shader_files;
    Shader raygen_narrow_shader;
    Shader raygen_wide_shader;
    Shader primary_miss_shader;
    Shader shadow_miss_shader;
    Shader shadow_intersection_shader;
    std::vector<Shader_group> shader_groups;

    bool mouse_control{ true }; // Mouse and controller can alternate for ui control

    Scene();

    void step();
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float>;

    Time_point m_start_clock = Clock::now();
};