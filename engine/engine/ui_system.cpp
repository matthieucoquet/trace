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
    // imgui input should be done before this call
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_vulkan_hpp";
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

    ImGui::Begin("##main_window", NULL, window_flags);

    ImGui::BeginChild("##scene_window", ImVec2(ImGui::GetWindowWidth() * 0.3f, 0.0f), true, ImGuiWindowFlags_None);

    if (ImGui::TreeNodeEx("Engine shaders", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int id = 0;
        for (auto& shader_file : scene.engine_shader_files)
        {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selected_shader == id) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(shader_file.name.c_str(), leaf_flags);
            if (ImGui::IsItemClicked()) {
                m_selected_engine = true;
                m_selected_shader = id;
                m_selected_object = std::numeric_limits<int>::max();
            }
            id++;
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Scene shaders", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int id = 0;
        for (auto& shader_file : scene.scene_shader_files)
        {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selected_shader == id) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(shader_file.name.c_str(), leaf_flags);
            if (ImGui::IsItemClicked()) {
                m_selected_engine = false;
                m_selected_shader = id;
                m_selected_object = std::numeric_limits<int>::max();
            }
            id++;
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (int id = 0; id < std::ssize(scene.objects); id++)
        {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selected_object == id) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(scene.objects[id].name.c_str(), leaf_flags);
            if (ImGui::IsItemClicked()) {
                m_selected_shader = std::numeric_limits<int>::max();
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

void Ui_system::shader_text(Shader_file& shader_file)
{
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize;
    ImGui::Text(shader_file.name.c_str());
    if (ImGui::InputTextMultiline(
        shader_file.name.c_str(), shader_file.data.data(), shader_file.data.size() + 1, // +1 for null terminated char
        ImVec2(-FLT_MIN, -FLT_MIN),
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
        shader_file.size = static_cast<int>(ptr_to_end - shader_file.data.data());
    }

}

void Ui_system::record_selected(Scene& scene)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::BeginChild("##shader_window", ImVec2(ImGui::GetWindowWidth() * 0.7f - 2 * style.WindowPadding.x - style.ItemSpacing.x, 0), true, ImGuiWindowFlags_None);

    if (m_selected_shader != std::numeric_limits<int>::max())
    {
        if (ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Shader"))
            {
                if (m_selected_engine) {
                    shader_text(scene.engine_shader_files[m_selected_shader]);
                }
                else {
                    shader_text(scene.scene_shader_files[m_selected_shader]);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Error"))
            {
                auto print_error = [](const Shader& shader) {
                    if (!shader.error.empty()) {
                        ImGui::TextWrapped(shader.error.c_str());
                    }
                };
                print_error(scene.raygen_narrow_shader);
                print_error(scene.raygen_wide_shader);
                print_error(scene.primary_miss_shader);
                print_error(scene.shadow_miss_shader);
                print_error(scene.shadow_intersection_shader);
                for (const auto& shader_group : scene.shader_groups) {
                    print_error(shader_group.primary_intersection);
                    print_error(shader_group.primary_closest_hit);
                    print_error(shader_group.shadow_any_hit);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    else if (m_selected_object != std::numeric_limits<int>::max())
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