#include "shader_system.hpp"
#include "core/scene.hpp"
#include "vulkan/context.hpp"

#include <fstream>
#include <iostream>
#include <ranges>
#include <fmt/core.h>
#include <marl/scheduler.h>
#include <marl/waitgroup.h>

class Includer : public shaderc::CompileOptions::IncluderInterface
{
public:
    Includer(Shader* shader, std::vector<Shader_file>* shader_files) : m_shader(shader), m_shader_files(shader_files)
    {}

    shaderc_include_result* GetInclude(
        const char* requested_source,
        shaderc_include_type /*type*/,
        const char* /*requesting_source*/,
        size_t /*include_depth*/) override final
    {
        auto file_it = std::find_if(m_shader_files->cbegin(), m_shader_files->cend(), [requested_source] (const Shader_file& shader_file) {
            return shader_file.name == requested_source;
        });
        assert(file_it != m_shader_files->cend());
        m_shader->included_files_id.push_back(std::distance(m_shader_files->cbegin(), file_it));
        const Shader_file& included_shader = *file_it;

        data_holder.content = included_shader.data.data();
        data_holder.content_length = included_shader.size;
        data_holder.source_name = included_shader.name.c_str();
        data_holder.source_name_length = included_shader.name.size();
        data_holder.user_data = nullptr;
        return &data_holder;
    }

    void ReleaseInclude(shaderc_include_result* /*data*/) override final {}
private:
    shaderc_include_result data_holder;
    Shader* m_shader;
    std::vector<Shader_file>* m_shader_files;
};

Shader_system::Shader_system(vulkan::Context& context, Scene& scene) :
    m_device(context.device),
    m_base_directory(SHADER_SOURCE)
{
    m_group_compile_options.SetOptimizationLevel(shaderc_optimization_level_performance);
    m_group_compile_options.SetWarningsAsErrors();
    m_group_compile_options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    m_group_compile_options.SetTargetSpirv(shaderc_spirv_version_1_5);
    for (auto& directory_entry : std::filesystem::directory_iterator(m_base_directory))
    {
        auto& path = directory_entry.path();
        scene.shader_files.push_back(Shader_file{
            .name = path.filename().string(),
            .data = read_file(path)
        });
        auto& back = scene.shader_files.back();
        back.size = back.data.size();
        m_shader_files_copy.push_back(Shader_file{
            .name = path.filename().string(),
            .data = scene.shader_files.back().data,
            .size = back.size
         });
    }
    auto find_file = [&scene](const auto& name) {
        auto file_it = std::find_if(scene.shader_files.cbegin(), scene.shader_files.cend(), [name](const Shader_file& shader_file) {
            return shader_file.name == name;
        });
        assert(file_it != scene.shader_files.cend());
        return std::distance(scene.shader_files.cbegin(), file_it);
    };
    scene.raygen_narrow_shader.shader_file_id = find_file("raygen_narrow.rgen");
    scene.raygen_wide_shader.shader_file_id = find_file("raygen_wide.rgen");
    scene.miss_shader.shader_file_id = find_file("miss.rmiss");
    scene.shadow_shader.shader_file_id = find_file("shadow.rmiss");
    for (auto& shader_group : scene.shader_groups) {
        shader_group.intersection.shader_file_id = find_file(shader_group.name + ".rint");
        shader_group.closest_hit.shader_file_id = find_file(shader_group.name + ".rchit");
    }

    marl::WaitGroup compile_shaders(static_cast<unsigned int>(scene.shader_groups.size()));
    for (auto& shader_group : scene.shader_groups) {
        marl::schedule([this, compile_shaders, &scene, &shader_group = shader_group]
        {
            compile(scene.shader_files, shader_group.intersection, shaderc_intersection_shader);
            compile(scene.shader_files, shader_group.closest_hit, shaderc_closesthit_shader);
            compile_shaders.done();
        });
    }
    compile(scene.shader_files, scene.raygen_narrow_shader, shaderc_raygen_shader);
    compile(scene.shader_files, scene.raygen_wide_shader, shaderc_raygen_shader);
    compile(scene.shader_files, scene.miss_shader, shaderc_miss_shader);
    compile(scene.shader_files, scene.shadow_shader, shaderc_miss_shader);
    compile_shaders.wait();
}

void Shader_system::step(Scene& scene)
{
    if (m_shaders_dirty)
    {
        if (!m_compiling.test(std::memory_order_relaxed)) {
            scene.pipeline_dirty = true;
            for (auto& shader_info : m_recompile_info) {
                if (!shader_info.copy.shader_module) {
                    scene.pipeline_dirty = false;
                }
                if (shader_info.original->shader_module) {
                    m_device.destroyShaderModule(shader_info.original->shader_module);
                }
                *shader_info.original = shader_info.copy;
            }
            m_shaders_dirty = false;
        }
    }
    else
    {
        size_t id = 0u;
        for (auto& file : scene.shader_files)
        {
            if (file.dirty) {
                m_shader_files_copy[id].data = file.data;
                m_shader_files_copy[id].size = file.size;
                m_shader_files_copy[id].dirty = true;
                file.dirty = false;
                m_shaders_dirty = true;
            }
            id++;
        }

        if (m_shaders_dirty)
        {
            m_compiling.test_and_set(std::memory_order_relaxed);
            marl::schedule([&scene, this]
            {
                m_recompile_info.clear();
                check_if_dirty(scene.raygen_narrow_shader, shaderc_raygen_shader);
                check_if_dirty(scene.raygen_wide_shader, shaderc_raygen_shader);
                check_if_dirty(scene.miss_shader, shaderc_miss_shader);
                check_if_dirty(scene.shadow_shader, shaderc_miss_shader);
                for (auto& shader_group : scene.shader_groups) {
                    check_if_dirty(shader_group.intersection, shaderc_intersection_shader);
                    check_if_dirty(shader_group.closest_hit, shaderc_closesthit_shader);
                }

                marl::WaitGroup compile_shaders(static_cast<unsigned int>(m_recompile_info.size()));
                for (auto& shader_info : m_recompile_info) {
                    marl::schedule([this, compile_shaders, &shader_info = shader_info]
                    {
                        compile(m_shader_files_copy, shader_info.copy, shader_info.kind);
                        compile_shaders.done();
                    });
                }
                compile_shaders.wait();
                m_compiling.clear(std::memory_order_release);
            });

            for (auto& file : scene.shader_files)
            {
                if (file.dirty) {
                    write_file(file);  // Do during destructor or multithread if too slow
                }
            }
        }
    }
}

void Shader_system::check_if_dirty(Shader& shader, shaderc_shader_kind shader_kind)
{
    if (m_shader_files_copy[shader.shader_file_id].dirty ||
        std::ranges::any_of(shader.included_files_id, [this](size_t id) { return m_shader_files_copy[id].dirty; })) {
        m_recompile_info.emplace_back(Recompile_info{
            .original = &shader,
            .copy = shader,
            .kind = shader_kind
        });
    }
}

void Shader_system::cleanup(Scene& scene)
{
    if (scene.raygen_narrow_shader.shader_module)
        m_device.destroyShaderModule(scene.raygen_narrow_shader.shader_module);
    if (scene.raygen_wide_shader.shader_module)
        m_device.destroyShaderModule(scene.raygen_wide_shader.shader_module);
    if (scene.miss_shader.shader_module)
        m_device.destroyShaderModule(scene.miss_shader.shader_module);
    if (scene.shadow_shader.shader_module)
        m_device.destroyShaderModule(scene.shadow_shader.shader_module);
    for (auto& shader_group : scene.shader_groups) {
        if (shader_group.intersection.shader_module)
            m_device.destroyShaderModule(shader_group.intersection.shader_module);
        if (shader_group.closest_hit.shader_module)
            m_device.destroyShaderModule(shader_group.closest_hit.shader_module);
    }
}

void Shader_system::compile(std::vector<Shader_file>& shader_files, Shader& shader, shaderc_shader_kind shader_kind)
{
    auto& shader_file = shader_files[shader.shader_file_id];
    shader.included_files_id.clear();
    auto group_compile_options = m_group_compile_options;
    group_compile_options.SetIncluder(std::make_unique<Includer>(&shader, &shader_files));
    auto compile_result = m_compiler.CompileGlslToSpv(shader_file.data.data(), shader_file.size, shader_kind, shader_file.name.c_str(), group_compile_options);
    if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        shader.error = compile_result.GetErrorMessage();
        shader.shader_module = vk::ShaderModule{};
    }
    else {
        shader.error = "";
        shader.shader_module = m_device.createShaderModule(vk::ShaderModuleCreateInfo{
            .codeSize = sizeof(shaderc::SpvCompilationResult::element_type) * std::distance(compile_result.begin(), compile_result.end()),
            .pCode = compile_result.begin()
            });
    }
}

std::string Shader_system::read_file(std::filesystem::path path) const
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
    std::string buffer("");
    buffer.resize(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}

void Shader_system::write_file(Shader_file shader_file)
{
    auto path = m_base_directory / shader_file.name;
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Shader path doesn't exist on filesystem.");
    }
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Shader path is not a regular file.");
    }

    std::ofstream file(path, std::ios::trunc | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    file.write(shader_file.data.data(), shader_file.size);
    //file.put('\r');
    //file.put('\n');
    fmt::print("write  {}\n", file.good());
    file.close();
}