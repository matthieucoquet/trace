#pragma once
#include <string>
#include <vector>
#include "vulkan/vk_common.hpp"

struct Shader_file
{
    bool dirty = false;
    std::string name;
    std::string data{};  // important to use a null terminating member for imgui
    int size{}; // null terminating character index
};

struct Shader
{
    int file_id;
    vk::ShaderModule module;
    std::vector<int> engine_included_id;
    std::vector<int> scene_included_id;
    std::string error;
};

struct  Shader_group
{
    std::string name;
    Shader primary_intersection;
    Shader primary_closest_hit;
    Shader shadow_any_hit;
};