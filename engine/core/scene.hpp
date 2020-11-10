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

struct Scene
{
    // Should probably be a runtime setting in the future
    static constexpr bool standing = false;
    static constexpr float vr_offset_y = standing ? 0.0f : 1.7f;

    Scene_global scene_global = {};

    std::vector<Object> objects;
    std::vector<glm::mat4> objects_transform;

    Object ui_object;

    std::vector<Material> materials;
    std::vector<Light> lights;

    bool pipeline_dirty = false;
    std::vector<Shader_file> engine_shader_files;
    std::vector<Shader_file> scene_shader_files;
    Shader raygen_shader;
    Shader primary_miss_shader;
    Shader shadow_miss_shader;
    Shader shadow_intersection_shader;
    std::vector<Shader_group> shader_groups;

    bool mouse_control{ true }; // Mouse and controller can alternate for ui control
};