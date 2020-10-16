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
    Includer(
        Shader& shader,
        const std::vector<Shader_file>& engine_files,
        const std::vector<Shader_file>& scene_files,
        const std::string& group_name) : 
        m_shader(shader), m_engine_files(engine_files), m_scene_files(scene_files), m_group_name(group_name)
    {}

    shaderc_include_result* GetInclude(
        const char* requested_source,
        shaderc_include_type /*type*/,
        const char* /*requesting_source*/,
        size_t /*include_depth*/) override final
    {
        if (!m_group_name.empty() && strcmp(requested_source, "map_function") == 0) {
            requested_source = m_group_name.c_str();
        }
        std::vector<Shader_file>::const_iterator file_it = std::find_if(m_scene_files.cbegin(), m_scene_files.cend(), [requested_source](const Shader_file& shader_file) {
            return shader_file.name == requested_source;
        });
        if (file_it != m_scene_files.cend()) {
            m_shader.scene_included_id.push_back(static_cast<int>(std::distance(m_scene_files.cbegin(), file_it)));
        }
        else {
            file_it = std::find_if(m_engine_files.cbegin(), m_engine_files.cend(), [requested_source](const Shader_file& shader_file) {
                return shader_file.name == requested_source;
            });
            assert(file_it != m_engine_files.cend());
            m_shader.engine_included_id.push_back(static_cast<int>(std::distance(m_engine_files.cbegin(), file_it)));
        }
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
    Shader& m_shader;
    const std::vector<Shader_file>& m_engine_files;
    const std::vector<Shader_file>& m_scene_files;
    const std::string& m_group_name;
};

Shader_system::Shader_system(vulkan::Context& context, Scene& scene, std::string_view scene_shader_path) :
    m_device(context.device),
    m_engine_directory(SHADER_SOURCE),
    m_scene_directory(scene_shader_path)
{
    //m_group_compile_options.SetOptimizationLevel(shaderc_optimization_level_performance);
    m_group_compile_options.SetOptimizationLevel(shaderc_optimization_level_zero);
    m_group_compile_options.SetWarningsAsErrors();
    m_group_compile_options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    m_group_compile_options.SetTargetSpirv(shaderc_spirv_version_1_5);

    // TODO Refactor
    for (auto& directory_entry : std::filesystem::directory_iterator(m_engine_directory))
    {
        auto& path = directory_entry.path();
        auto& back = scene.engine_shader_files.emplace_back(Shader_file{
            .name = path.filename().string(),
            .data = read_file(path)
        });
        back.size = static_cast<int>(std::ssize(back.data));
        m_engine_files_copy.push_back(Shader_file{
            .name = path.filename().string(),
            .data = scene.engine_shader_files.back().data,
            .size = back.size
         });
    }
    for (auto& directory_entry : std::filesystem::directory_iterator(m_scene_directory))
    {
        auto& path = directory_entry.path();
        auto& back = scene.scene_shader_files.emplace_back(Shader_file{
            .name = path.filename().string(),
            .data = read_file(path)
            });
        back.size = static_cast<int>(std::ssize(back.data));
        m_scene_files_copy.push_back(Shader_file{
            .name = path.filename().string(),
            .data = scene.scene_shader_files.back().data,
            .size = back.size
            });
    }
    auto find_file = [&scene](const auto& name) {
        auto file_it = std::find_if(scene.engine_shader_files.cbegin(), scene.engine_shader_files.cend(), [name](const Shader_file& shader_file) {
            return shader_file.name == name;
        });
        assert(file_it != scene.engine_shader_files.cend());
        return static_cast<int>(std::distance(scene.engine_shader_files.cbegin(), file_it));
    };
    scene.raygen_narrow_shader.file_id = find_file("raygen_narrow.rgen");
    scene.raygen_wide_shader.file_id = find_file("raygen_wide.rgen");
    scene.primary_miss_shader.file_id = find_file("primary.rmiss");
    scene.shadow_miss_shader.file_id = find_file("shadow.rmiss");
    scene.shadow_intersection_shader.file_id = find_file("shadow.rint");
    for (auto& shader_group : scene.shader_groups) {
        shader_group.primary_intersection.file_id = find_file("primary.rint");
        shader_group.primary_closest_hit.file_id = find_file("primary.rchit");
        shader_group.shadow_any_hit.file_id = find_file("shadow.rahit");
    }

    marl::WaitGroup compile_shaders(static_cast<unsigned int>(scene.shader_groups.size()));
    for (auto& shader_group : scene.shader_groups) {
        marl::schedule([this, compile_shaders, &scene, &shader_group = shader_group]
        {
            compile(scene.engine_shader_files, scene.scene_shader_files, shader_group.primary_intersection, shaderc_intersection_shader, shader_group.name);
            compile(scene.engine_shader_files, scene.scene_shader_files, shader_group.primary_closest_hit, shaderc_closesthit_shader, shader_group.name);
            compile(scene.engine_shader_files, scene.scene_shader_files, shader_group.shadow_any_hit, shaderc_anyhit_shader, shader_group.name);
            compile_shaders.done();
        });
    }
    compile(scene.engine_shader_files, scene.scene_shader_files, scene.raygen_narrow_shader, shaderc_raygen_shader);
    compile(scene.engine_shader_files, scene.scene_shader_files, scene.raygen_wide_shader, shaderc_raygen_shader);
    compile(scene.engine_shader_files, scene.scene_shader_files, scene.primary_miss_shader, shaderc_miss_shader);
    compile(scene.engine_shader_files, scene.scene_shader_files, scene.shadow_miss_shader, shaderc_miss_shader);
    compile(scene.engine_shader_files, scene.scene_shader_files, scene.shadow_intersection_shader, shaderc_intersection_shader);
    compile_shaders.wait();
}

void Shader_system::step(Scene& scene)
{
    if (m_shaders_dirty)
    {
        if (!m_compiling.test(std::memory_order_relaxed)) {
            scene.pipeline_dirty = true;
            for (auto& shader_info : m_recompile_info) {
                if (!shader_info.copy.module) {
                    scene.pipeline_dirty = false;
                }
                if (shader_info.original->module) {
                    m_device.destroyShaderModule(shader_info.original->module);
                }
                *shader_info.original = shader_info.copy;
            }
            m_shaders_dirty = false;
        }
    }
    else
    {
        size_t id = 0u;
        for (auto& file : scene.engine_shader_files)
        {
            if (file.dirty) {
                m_engine_files_copy[id] = file;
                m_shaders_dirty = true;
            }
            id++;
        }
        id = 0u;
        for (auto& file : scene.scene_shader_files)
        {
            if (file.dirty) {
                m_scene_files_copy[id] = file;
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
                check_if_dirty(scene.primary_miss_shader, shaderc_miss_shader);
                check_if_dirty(scene.shadow_miss_shader, shaderc_miss_shader);
                check_if_dirty(scene.shadow_intersection_shader, shaderc_intersection_shader);
                for (auto& shader_group : scene.shader_groups) {
                    check_if_dirty(shader_group.primary_intersection, shaderc_intersection_shader, shader_group.name);
                    check_if_dirty(shader_group.primary_closest_hit, shaderc_closesthit_shader, shader_group.name);
                    check_if_dirty(shader_group.shadow_any_hit, shaderc_anyhit_shader, shader_group.name);
                }

                marl::WaitGroup compile_shaders(static_cast<unsigned int>(m_recompile_info.size()));
                for (auto& shader_info : m_recompile_info) {
                    marl::schedule([this, compile_shaders, &shader_info = shader_info]
                    {
                        compile(m_engine_files_copy, m_scene_files_copy, shader_info.copy, shader_info.kind, shader_info.name);
                        compile_shaders.done();
                    });
                }
                compile_shaders.wait();
                m_compiling.clear(std::memory_order_release);
            });

            for (auto& file : scene.engine_shader_files)
            {
                if (file.dirty) {
                    write_file(file, true);  // Do during destructor or multithread if too slow
                    file.dirty = false;
                }
            }
            for (auto& file : scene.scene_shader_files)
            {
                if (file.dirty) {
                    write_file(file, false);
                    file.dirty = false;
                }
            }
        }
    }
}

void Shader_system::check_if_dirty(Shader& shader, shaderc_shader_kind shader_kind, const std::string& group_name)
{
    if (m_engine_files_copy[shader.file_id].dirty ||
        std::ranges::any_of(shader.engine_included_id, [this](size_t id) { return m_engine_files_copy[id].dirty; }) ||
        std::ranges::any_of(shader.scene_included_id, [this](size_t id) { return m_scene_files_copy[id].dirty; }))
    {
        m_recompile_info.emplace_back(Recompile_info{
            .original = &shader,
            .copy = shader,
            .kind = shader_kind,
            .name = group_name
        });
    }
}

void Shader_system::cleanup(Scene& scene)
{
    if (scene.raygen_narrow_shader.module)
        m_device.destroyShaderModule(scene.raygen_narrow_shader.module);
    if (scene.raygen_wide_shader.module)
        m_device.destroyShaderModule(scene.raygen_wide_shader.module);
    if (scene.primary_miss_shader.module)
        m_device.destroyShaderModule(scene.primary_miss_shader.module);
    if (scene.shadow_miss_shader.module)
        m_device.destroyShaderModule(scene.shadow_miss_shader.module);
    if (scene.shadow_intersection_shader.module)
        m_device.destroyShaderModule(scene.shadow_intersection_shader.module);
    for (auto& shader_group : scene.shader_groups) {
        if (shader_group.primary_intersection.module)
            m_device.destroyShaderModule(shader_group.primary_intersection.module);
        if (shader_group.primary_closest_hit.module)
            m_device.destroyShaderModule(shader_group.primary_closest_hit.module);
        if (shader_group.shadow_any_hit.module)
            m_device.destroyShaderModule(shader_group.shadow_any_hit.module);
    }
}

void Shader_system::compile(
    const std::vector<Shader_file>& engine_shader_files,
    const std::vector<Shader_file>& scene_shader_files,
    Shader& shader, shaderc_shader_kind shader_kind, const std::string& group_name)
{
    auto& shader_file = engine_shader_files[shader.file_id];
    shader.engine_included_id.clear();
    shader.scene_included_id.clear();
    auto group_compile_options = m_group_compile_options;

    std::string group_name_file(group_name + ".glsl");
    group_compile_options.SetIncluder(std::make_unique<Includer>(shader, engine_shader_files, scene_shader_files, group_name_file));
    auto compile_result = m_compiler.CompileGlslToSpv(shader_file.data.data(), shader_file.size, shader_kind, shader_file.name.c_str(), group_compile_options);
    if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        shader.error = compile_result.GetErrorMessage();
        shader.module = vk::ShaderModule{};
        fmt::print("GLSL compilation error: {}\n", shader.error);
    }
    else {
        shader.error = "";
        shader.module = m_device.createShaderModule(vk::ShaderModuleCreateInfo{
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

void Shader_system::write_file(Shader_file shader_file, bool engine_shader)
{
    auto path = (engine_shader ? m_engine_directory : m_scene_directory) / shader_file.name;
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
    file.close();
}