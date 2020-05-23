#include "shader_compile.h"

#include <fstream>
#include <iostream>
#include <memory>

namespace vulkan
{

std::vector<char> read_file(std::filesystem::path path)
{
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Shader path doesn't exist on filesystem.");
    }
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Shader path is not a regular file.");
    }

    std::ifstream file(path.string(), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}

class Includer : public shaderc::CompileOptions::IncluderInterface
{
public:
    Includer(std::filesystem::path base_directory) : m_base_directory(std::move(base_directory))
    {}

    shaderc_include_result* GetInclude(
        const char* requested_source,
        shaderc_include_type /*type*/,
        const char* /*requesting_source*/,
        size_t /*include_depth*/) override final
    {
        auto file_path = m_base_directory / requested_source;

        Shaderc_result_holder* holder = new Shaderc_result_holder();
        holder->content = read_file(file_path);
        holder->name = file_path.string();

        holder->result.content = holder->content.data();
        holder->result.content_length = holder->content.size();
        holder->result.source_name = holder->name.c_str();
        holder->result.source_name_length = holder->name.size();
        holder->result.user_data = reinterpret_cast<void*>(holder);
        return &holder->result;
    }

    void ReleaseInclude(shaderc_include_result* data) override final
    {
        delete reinterpret_cast<Shaderc_result_holder*>(data->user_data);
    }
private:
    struct Shaderc_result_holder
    {
        std::string name;
        std::vector<char> content;
        shaderc_include_result result;
    };

    std::filesystem::path m_base_directory;
};

Shader_compile::Shader_compile():
    m_base_directory(SHADER_SOURCE)  // Set from CMake
{
    m_group_compile_options.SetIncluder(std::make_unique<Includer>(m_base_directory));
}

vk::ShaderModule Shader_compile::compile(vk::Device device, const std::filesystem::path& file_name, shaderc_shader_kind shader_kind, shaderc::CompileOptions options)
{
    auto file_path = m_base_directory / file_name;

    auto data = read_file(file_path);
    auto compile_result = m_compiler.CompileGlslToSpv(data.data(), data.size(), shader_kind, file_path.string().c_str(), options);
    if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cout << compile_result.GetErrorMessage() << std::endl;
    }

    return device.createShaderModule(vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(shaderc::SpvCompilationResult::element_type) * std::distance(compile_result.begin(), compile_result.end()))
        .setPCode(compile_result.begin()));
}

Shader_group Shader_compile::compile_group(vk::Device device, const char* group_name)
{
    auto intersection_path = std::filesystem::path(group_name);
    intersection_path.replace_extension("rint");
    auto closest_hit_path = std::filesystem::path(group_name);
    closest_hit_path.replace_extension("rchit");

    return Shader_group
    {
        .intersection = compile(device, intersection_path, shaderc_intersection_shader, m_group_compile_options),
        .closest_hit = compile(device, closest_hit_path, shaderc_closesthit_shader, m_group_compile_options),
    };
}

}
