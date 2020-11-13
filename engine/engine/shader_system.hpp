#pragma once
#include "core/scene.hpp"
#include "core/system.hpp"
#include "core/shader_data.hpp"
#include <filesystem>
#include <shaderc/shaderc.hpp>
#include <marl/scheduler.h>

namespace vulkan {
class Context;
}

class Shader_system final : public System
{
public:
    Shader_system(vulkan::Context& context, Scene& scene, std::string_view scene_shader_path);
    Shader_system(const Shader_system& other) = delete;
    Shader_system(Shader_system&& other) = delete;
    Shader_system& operator=(const Shader_system& other) = delete;
    Shader_system& operator=(Shader_system&& other) = delete;
    ~Shader_system() override = default;
    void step(Scene& scene) override final;
    void cleanup(Scene& scene)  override final;
private:
    struct Recompile_info {
        Shader* original;
        Shader copy;
        shaderc_shader_kind kind;
        std::string name;  // TODO stringview
    };

    vk::Device m_device;
    shaderc::Compiler m_compiler;
    std::filesystem::path m_engine_directory;
    std::filesystem::path m_scene_directory;
    shaderc::CompileOptions m_group_compile_options;

    marl::Scheduler m_scheduler{ marl::Scheduler::Config::allCores() };

    // For multithread access
    std::vector<Shader_file> m_engine_files_copy;
    std::vector<Shader_file> m_scene_files_copy;
    std::vector<Recompile_info> m_recompile_info;
    std::atomic_flag m_compiling;
    bool m_shaders_dirty = false;

    void compile(
        const std::vector<Shader_file>& engine_shader_files,
        const std::vector<Shader_file>& scene_shader_files,
        Shader& shader, shaderc_shader_kind shader_kind, const std::string& group_name = {});
    [[nodiscard]] std::string read_file(std::filesystem::path path) const;
    void write_file(Shader_file shader_file, bool engine_shader);
    void check_if_dirty(Shader& shader, shaderc_shader_kind shader_kind, const std::string& group_name = {});

};
