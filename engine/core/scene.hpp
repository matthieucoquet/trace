#pragma once
#include "vulkan/vk_common.hpp"
#include "vr/vr_common.hpp"
#include <array>
#include <vector>
#include <chrono>
#include <filesystem>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "shader.hpp"
#include "transform.hpp"

namespace sdf_editor
{

struct Eye
{
    xr::Posef pose = {};
    xr::Fovf fov = {};
};

struct Scene_global
{
    glm::mat4 transform;
    std::array<Eye, 2> eyes;
    float time = {};
    int nb_lights = {};
};

struct Material
{
    glm::vec4 color;
    float ks;
    float shininess;
    float f0;
};

struct Light
{
    glm::vec3 local;
    glm::vec3 global;
    glm::vec3 color;

    void update(const Transform& parent) {
        global = parent.position + glm::rotate(parent.rotation, parent.scale * local);
    }
};

struct Entity
{
    static constexpr size_t scene_id = std::numeric_limits<size_t>::max();
    static constexpr  size_t empty_id = scene_id - 1u;

    std::string name;

    Transform local_transform{};
    Transform global_transform{};

    size_t group_id;

    std::vector<Entity> children{};

    int hand_grabbing = -1;
    bool dirty_global = false;
    bool dirty_local = false;

    template<typename F>
    void visit(F func) {
        func(*this);
        for (auto& child : children) {
            child.visit(func);
        }
    }
};

struct Scene
{
    // Should probably be a runtime setting in the future
    static constexpr bool standing = true;
    static constexpr float vr_offset_y = standing ? 0.0f : 1.7f;
    static constexpr unsigned int max_entities = 20u;
    static constexpr unsigned int max_lights = 10u;
    bool mouse_control{ true }; // Mouse and controller can alternate for ui control

    Scene_global scene_global = {};

    std::vector<Entity> entities{};
    std::vector<vk::AccelerationStructureInstanceKHR> entities_instances{};

    std::vector<Material> materials;
    std::vector<Light> lights;

    std::filesystem::path texture_path{};

    Shaders shaders;

    glm::vec3 camera_position{}; // For desktop mode
    float camera_rot_y{};
    float camera_rot_z{};

    bool saving{ false };
    bool resetting{ false };
};

}