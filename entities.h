#pragma once
#include <map>
#include <string>
#include <vector>
#include "vulkan/vk_common.h"

struct Shader_file
{
    bool dirty = false;
    std::string name;
    std::vector<char> data{};
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