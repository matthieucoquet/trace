#pragma once
#include <string>
#include <vector>
#include "vulkan/vk_common.hpp"

struct Shader_file
{
    bool dirty = false;
    std::string name;
    std::string data{};  // important to use a null terminating member for imgui
    size_t size{}; // null terminating character index
};

struct Shader
{
    size_t shader_file_id;
    vk::ShaderModule shader_module;
    std::vector<size_t> included_files_id;
    std::string error;
};

struct  Shader_group
{
    std::string name;
    Shader primary_intersection;
    Shader primary_closest_hit;
    Shader shadow_any_hit;
};