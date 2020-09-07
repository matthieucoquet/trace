#pragma once
#include <string>
#include <vector>
#include "vulkan/vk_common.hpp"

struct Shader_file
{
    bool dirty = false;
    std::string name;
    std::string data{};  // important to use a null terminating member for imgui
};

struct Shader
{
    size_t shader_file_id;
    vk::ShaderModule shader_module;
    std::vector<size_t> included_files_id;
    std::string error;
};

struct Shader_group
{
    std::string name;
    Shader intersection;
    Shader closest_hit;
};