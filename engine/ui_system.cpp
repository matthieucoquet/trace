#include "ui_system.hpp"
#include "core/scene.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fmt/core.h>

Ui_system::Ui_system()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
}

void Ui_system::step(Scene& scene)
{
    if (!m_selected_shader && m_selected_object == std::numeric_limits<size_t>::max()) {
        m_selected_shader = &scene.raygen_narrow_shader;
    }
    // imgui input should be done before this call
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_vulkan_hpp";
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

    ImGui::Begin("##main_window", NULL, window_flags);

    ImGui::BeginChild("##scene_window", ImVec2(ImGui::GetWindowWidth() * 0.3f, 0.0f), true, ImGuiWindowFlags_None);

    if (ImGui::TreeNodeEx("Shaders", ImGuiTreeNodeFlags_DefaultOpen))
    {
        add_leaf("Raygen narrow", &scene.raygen_narrow_shader);
        add_leaf("Raygen wide", &scene.raygen_wide_shader);
        add_leaf("Miss", &scene.miss_shader);

        for (auto& shader_group : scene.shader_groups)
        {
            if (ImGui::TreeNode(shader_group.name.c_str()))
            {
                add_leaf("Intersection", &shader_group.intersection);
                add_leaf("Closest hit", &shader_group.closest_hit);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (size_t id = 0u; id < scene.objects.size(); id++)
        {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selected_object == id) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(scene.objects[id].name.c_str(), leaf_flags);
            if (ImGui::IsItemClicked()) {
                m_selected_shader = nullptr;
                m_selected_object = id;
            }
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();

    ImGui::SameLine();
    record_selected(scene);
    ImGui::End();
    ImGui::Render();
}

void Ui_system::add_leaf(const char* label, Shader* shader)
{
    ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (m_selected_shader == shader) {
        leaf_flags |= ImGuiTreeNodeFlags_Selected;
    }
    ImGui::TreeNodeEx(label, leaf_flags);
    if (ImGui::IsItemClicked()) {
        m_selected_shader = shader;
        m_selected_object = std::numeric_limits<size_t>::max();
    }
}

void Ui_system::shader_text(Shader_file& shader_file)
{
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize;
    ImGui::Text(shader_file.name.c_str());
    if (ImGui::InputTextMultiline(
        shader_file.name.c_str(), shader_file.data.data(), shader_file.data.size() + 1, // +1 for null terminated char
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 14),
        flags, [](ImGuiInputTextCallbackData* data)
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                auto* string = static_cast<std::string*>(data->UserData);
                string->resize(data->BufTextLen);
                data->Buf = string->data();
            }
            return 0;
        }, (void*)&shader_file.data))
    {
        shader_file.dirty = true;
        char* ptr_to_end = strchr(shader_file.data.data(), '\0');
        shader_file.size = ptr_to_end - shader_file.data.data();
    }

}

void Ui_system::record_selected(Scene& scene)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::BeginChild("##shader_window", ImVec2(ImGui::GetWindowWidth() * 0.7f - 2 * style.WindowPadding.x - style.ItemSpacing.x, 0), true, ImGuiWindowFlags_None);

    if (m_selected_shader)
    {
        for (auto child_id : m_selected_shader->included_files_id)
        {
            shader_text(scene.shader_files[child_id]);
        }
        shader_text(scene.shader_files[m_selected_shader->shader_file_id]);
        if (!m_selected_shader->error.empty()) {
            ImGui::Text("Compilation error");
            ImGui::TextWrapped(m_selected_shader->error.c_str());
        }
    }
    else if (m_selected_object != std::numeric_limits<size_t>::max())
    {
        Object& object = scene.objects[m_selected_object];
        bool dirty = ImGui::InputFloat3("Position", glm::value_ptr(object.position));
        dirty =! ImGui::InputFloat4("Rotation", glm::value_ptr(object.rotation));
        dirty =! ImGui::SliderFloat("Scale", &object.scale, 0.03f, 5.0f, "%.3f");
        if (dirty)
        {
            glm::mat4 model_to_world = glm::translate(object.position) * glm::toMat4(object.rotation) * glm::scale(glm::vec3(object.scale));
            scene.objects_transform[m_selected_object] = glm::inverse(model_to_world);
        }
    }
    ImGui::EndChild();

}
