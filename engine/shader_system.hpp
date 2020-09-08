#pragma once
#include "core/system.hpp"
#include "core/shader_data.hpp"
#include <filesystem>
#include <shaderc/shaderc.hpp>

namespace vulkan {
class Context;
}

class Shader_system : public System
{
public:
    Shader_system(vulkan::Context& context, Scene& scene);
    Shader_system(const Shader_system& other) = delete;
    Shader_system(Shader_system&& other) = delete;
    Shader_system& operator=(const Shader_system& other) = delete;
    Shader_system& operator=(Shader_system&& other) = delete;
    ~Shader_system() override final = default;
    void step(Scene& scene) override final;
    void cleanup(Scene& scene)  override final;
private:
    struct Recompile_info {
        Shader* original;
        Shader copy;
        shaderc_shader_kind kind;
    };

    vk::Device m_device;
    shaderc::Compiler m_compiler;
    std::filesystem::path m_base_directory;
    shaderc::CompileOptions m_group_compile_options;

    // For multithread access
    std::vector<Shader_file> m_shader_files_copy;
    std::vector<Recompile_info> m_recompile_info;
    std::atomic_flag m_compiling;
    bool m_shaders_dirty;

    void compile(std::vector<Shader_file>& shader_files, Shader& shader, shaderc_shader_kind shader_kind);
    [[nodiscard]] std::string read_file(std::filesystem::path path) const;
    void check_if_dirty(Shader& shader, shaderc_shader_kind shader_kind);

};