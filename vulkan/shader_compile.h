#pragma once

#include <filesystem>
#include <shaderc/shaderc.hpp>
#include "common.h"

namespace vulkan
{

struct Shader_group
{
	vk::ShaderModule intersection;
	vk::ShaderModule closest_hit;
};

class Shader_compile
{
public:
	Shader_compile();
	vk::ShaderModule compile(vk::Device device, const std::filesystem::path& file_name, shaderc_shader_kind shader_kind, shaderc::CompileOptions options = {});
	Shader_group compile_group(vk::Device device, const char* group_name);
private:
	shaderc::Compiler m_compiler;
	std::filesystem::path m_base_directory;
	shaderc::CompileOptions m_group_compile_options;
};

}