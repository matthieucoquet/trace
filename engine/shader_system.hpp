#pragma once
#include "core/system.hpp"
#include "core/entities.hpp"
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
    vk::Device m_device;
    shaderc::Compiler m_compiler;
    std::filesystem::path m_base_directory;
    shaderc::CompileOptions m_group_compile_options;

    void compile(Scene& scene, Shader& shader, shaderc_shader_kind shader_kind);
    [[nodiscard]] std::vector<char> read_file(std::filesystem::path path) const;
};