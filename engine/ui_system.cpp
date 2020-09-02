#include "ui_system.hpp"
#include "core/scene.hpp"
#include <imgui.h>

Ui_system::Ui_system()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
}

void Ui_system::step(Scene& scene)
{
    if (!m_selected_shader) {
        m_selected_shader = &scene.raygen_center_shader;
    }
    // imgui input should be done before this call
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_vulkan_hpp";
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
    ImGuiWindowFlags children_flags = ImGuiWindowFlags_None;

    ImGui::Begin("##main_window", NULL, window_flags);

    ImGui::BeginChild("##scene_window", ImVec2(ImGui::GetWindowWidth() * 0.3f, 0.0f), true, children_flags);

    if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
        add_leaf("Raygen", &scene.raygen_center_shader);
        add_leaf("Raygen", &scene.raygen_side_shader);
        add_leaf("Miss", &scene.miss_shader);

        for (auto& entity: scene.entities)
        {
            if (ImGui::TreeNode(entity.name.c_str()))
            {
                add_leaf("Intersection", &entity.intersection);
                add_leaf("Closest hit", &entity.closest_hit);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
            
            
    ImGui::EndChild();

    ImGui::SameLine();
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::BeginChild("##shader_window", ImVec2(ImGui::GetWindowWidth() * 0.7f - 2 * style.WindowPadding.x - style.ItemSpacing.x , 0), true, children_flags);

    for (auto child_id : m_selected_shader->included_files_id)
    {
        shader_text(scene.shader_files[child_id]);
    }
    shader_text(scene.shader_files[m_selected_shader->shader_file_id]);
    if (!m_selected_shader->error.empty()) {
        ImGui::Text("Compilation error");
        ImGui::TextWrapped(m_selected_shader->error.c_str()); 
    }

    ImGui::EndChild();
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
    }
}

void Ui_system::shader_text(Shader_file& shader_file)
{
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize;
    ImGui::Text(shader_file.name.c_str());
    ImGui::InputTextMultiline(
        shader_file.name.c_str(), shader_file.data.data(), shader_file.data.size(), 
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 10), 
        flags, [](ImGuiInputTextCallbackData* data)
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                auto* string = static_cast<std::vector<char>*>(data->UserData);
                string->resize(data->BufSize);
                data->Buf = string->data();
            }
            return 0;
        }, (void*)&shader_file.data);

}