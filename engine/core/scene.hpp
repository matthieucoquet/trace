#pragma once
#include "device_data.hpp"
#include "shader_data.hpp"
#include "character.hpp"
#include <vector>
#include <chrono>

struct Object
{
    std::string name;
    // TODO use transform
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
    bool mouse_control{ true }; // Mouse and controller can alternate for ui control

    Scene_global scene_global = {};

    Object ui_object;
    std::vector<Object> objects;
    std::vector<glm::mat4> objects_transform;

    std::vector<Character> characters;

    std::vector<Material> materials;
    std::vector<Light> lights;

    Shaders shaders;
};