#include "ui_system.h"
#include "scene.h"
#include <imgui.h>

void Ui_system::step(Scene& /*scene*/)
{
    ImGui::NewFrame();

    ImGui::Begin("Dear ImGui Demo");

    ImGui::Text("Wow.");
    /*if (ImGui::TreeNode("Scene"))
    {
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf;
        ImGui::TreeNodeEx("raygen_entity", node_flags, "Raygen");
        if (ImGui::IsItemClicked()) {}
        ImGui::TreeNodeEx("miss_entity", node_flags, "Miss");
        if (ImGui::IsItemClicked()) {}
    }*/

    //(const char* label, char* buf, size_t buf_size, const ImVec2 & size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
    // TODO set mamber ?
    //ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly;
    //auto& shader_file = scene.shader_files[scene.raygen_shader.shader_file_id];
    //ImGui::InputTextMultiline("sources", shader_file.data.data(), shader_file.data.size(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 10), flags);

    ImGui::End();
    ImGui::Render();
}