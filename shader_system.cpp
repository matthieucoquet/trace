#include "shader_system.h"
#include "core/scene.h"
#include "vulkan/context.h"
#include <fstream>
#include <iostream>

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
        data_holder.content_length = included_shader.data.size();
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
    //m_group_compile_options.SetOptimizationLevel(shaderc_optimization_level_zero);
    m_group_compile_options.SetWarningsAsErrors();
    m_group_compile_options.SetTargetSpirv(shaderc_spirv_version_1_5);
    for (auto& directory_entry : std::filesystem::directory_iterator(m_base_directory))
    {
        auto& path = directory_entry.path();
        scene.shader_files.push_back(Shader_file{
            .name = path.filename().string(),
            .data = read_file(path)
        });
    }
    auto find_file = [&scene](const auto& name) {
        auto file_it = std::find_if(scene.shader_files.cbegin(), scene.shader_files.cend(), [name](const Shader_file& shader_file) {
            return shader_file.name == name;
        });
        assert(file_it != scene.shader_files.cend());
        return std::distance(scene.shader_files.cbegin(), file_it);
    };
    scene.raygen_center_shader.shader_file_id = find_file("raygen.rgen");
    compile(scene, scene.raygen_center_shader, shaderc_raygen_shader);
    scene.raygen_side_shader.shader_file_id = find_file("raygen_side.rgen");
    compile(scene, scene.raygen_side_shader, shaderc_raygen_shader);
    scene.miss_shader.shader_file_id = find_file("miss.rmiss");
    compile(scene, scene.miss_shader, shaderc_miss_shader);

    for (auto& entity : scene.entities) {
        entity.intersection.shader_file_id = find_file(entity.name + ".rint");
        compile(scene, entity.intersection, shaderc_intersection_shader);
        entity.closest_hit.shader_file_id = find_file(entity.name + ".rchit");
        compile(scene, entity.closest_hit, shaderc_closesthit_shader);
    }
}

void Shader_system::step(Scene& /*scene*/)
{

}

void Shader_system::cleanup(Scene& scene)
{
    m_device.destroyShaderModule(scene.raygen_center_shader.shader_module);
    m_device.destroyShaderModule(scene.raygen_side_shader.shader_module);
    m_device.destroyShaderModule(scene.miss_shader.shader_module);
    for (auto& entity : scene.entities) {
        m_device.destroyShaderModule(entity.intersection.shader_module);
        m_device.destroyShaderModule(entity.closest_hit.shader_module);
    }
}

void Shader_system::compile(Scene& scene, Shader& shader, shaderc_shader_kind shader_kind)
{
    auto& shader_file = scene.shader_files[shader.shader_file_id];
    shader.included_files_id.clear();
    m_group_compile_options.SetIncluder(std::make_unique<Includer>(&shader, &scene.shader_files));
    auto compile_result = m_compiler.CompileGlslToSpv(shader_file.data.data(), shader_file.data.size(), shader_kind, shader_file.name.c_str(), m_group_compile_options);
    if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        shader.error = compile_result.GetErrorMessage();
    }
    shader.shader_module = m_device.createShaderModule(vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(shaderc::SpvCompilationResult::element_type) * std::distance(compile_result.begin(), compile_result.end()))
        .setPCode(compile_result.begin()));
}

std::vector<char> Shader_system::read_file(std::filesystem::path path) const
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