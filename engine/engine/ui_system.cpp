#include "ui_system.hpp"
#include "core/scene.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fmt/core.h>

namespace sdf_editor
{

Ui_system::Ui_system()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
}

void Ui_system::step(Scene& scene)
{
    scene.saving = false;
    scene.resetting = false;
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
        for (auto& shader_file : scene.shaders.engine_files)
        {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selected == Selected::engine_shader && m_selected_id == id) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(shader_file.name.c_str(), leaf_flags);
            if (ImGui::IsItemClicked()) {
                m_selected = Selected::engine_shader;
                m_selected_id = id;
            }
            id++;
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Scene shaders", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int id = 0;
        for (auto& shader_file : scene.shaders.scene_files)
        {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selected == Selected::scenes_shader && m_selected_id == id) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(shader_file.name.c_str(), leaf_flags);
            if (ImGui::IsItemClicked()) {
                m_selected = Selected::scenes_shader;
                m_selected_id = id;
            }
            id++;
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNodeEx("Entities", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int id = -1;
        for (auto& entity : scene.entities)
        {
            id = entity_node(entity, id + 1);
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (int id = 0; id < std::ssize(scene.materials); id++)
        {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selected == Selected::material && m_selected_id == id) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(fmt::format("material_{}", id).c_str(), leaf_flags);
            if (ImGui::IsItemClicked()) {
                m_selected = Selected::material;
                m_selected_id = id;
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (int id = 0; id < std::ssize(scene.lights); id++)
        {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selected == Selected::light && m_selected_id == id) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(fmt::format("light_{}", id).c_str(), leaf_flags);
            if (ImGui::IsItemClicked()) {
                m_selected = Selected::light;
                m_selected_id = id;
            }
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::Button("Save")) {
        scene.saving = true;
    }
    ImGui::SameLine();;
    if (ImGui::Button("Reset")) {
        scene.resetting = true;
    }
    ImGui::EndChild();

    ImGui::SameLine();
    record_selected(scene);
    ImGui::End();
    ImGui::Render();
}

int Ui_system::entity_node(Entity& entity, int id)
{
    //ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    ImGuiTreeNodeFlags flags = entity.children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : ImGuiTreeNodeFlags_DefaultOpen;
    if (m_selected == Selected::entity && m_selected_id == id) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (ImGui::TreeNodeEx(entity.name.c_str(), flags))
    {
        if (ImGui::IsItemClicked()) {
            m_selected = Selected::entity;
            m_selected_id = id;
        }

        for (auto& child : entity.children)
        {
            id = entity_node(child, id + 1);
        }
        if (!entity.children.empty()) {
            ImGui::TreePop();
        }
    }
    return id;
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

    switch (m_selected)
    {
    case Selected::engine_shader:
    case Selected::scenes_shader:
    {
        if (ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Shader"))
            {
                if (m_selected == Selected::engine_shader) {
                    shader_text(scene.shaders.engine_files[m_selected_id]);
                }
                else {
                    shader_text(scene.shaders.scene_files[m_selected_id]);
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
                print_error(scene.shaders.raygen);
                print_error(scene.shaders.primary_miss);
                print_error(scene.shaders.shadow_miss);
                print_error(scene.shaders.shadow_intersection);
                for (const auto& shader_group : scene.shaders.groups) {
                    print_error(shader_group.primary_intersection);
                    print_error(shader_group.primary_closest_hit);
                    print_error(shader_group.shadow_any_hit);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        break;
    }
    case Selected::entity:
    {
        Entity* selected = nullptr;
        int id = 0;
        for (auto& entity : scene.entities)
        {
            entity.visit([this, &id, &selected](Entity& entity) {
                if (m_selected_id == id) {
                    selected = &entity;
                }
                id++;
            });
        }
        if (selected) {
            bool dirty = ImGui::InputFloat3("Position", glm::value_ptr(selected->local_transform.position));
            dirty = dirty | ImGui::InputFloat4("Rotation", glm::value_ptr(selected->local_transform.rotation));
            dirty = dirty | ImGui::SliderFloat("Scale", &selected->local_transform.scale, 0.03f, 5.0f, "%.3f");
            if (dirty)
            {
                selected->dirty_global = true;
            }
            if (ImGui::Button("Remove")) {
                for (auto& entity : scene.entities)
                {
                    entity.visit([this, &scene, &selected](Entity& entity) {
                        bool found = false;
                        for (int i = 0; i < entity.children.size(); i++) {
                            if (&entity.children[i] == selected) {
                                found = true;
                            }
                            if (found && (i + 1 < entity.children.size())) {
                                std::swap(entity.children[i], entity.children[i + 1]);
                            }
                        }
                        if (found) {
                            entity.children.pop_back();
                            scene.entities_instances.pop_back();
                        }
                     });
                }
                scene.entities[3].dirty_global = true;
            }
            if (ImGui::Button("Add new")) {
                selected->children.emplace_back(Entity{
                    .dirty_global = true
                });
            }
        }
        break;
    }
    case Selected::material:
    {
        Material& material = scene.materials[m_selected_id];
        ImGui::ColorPicker3("Color", glm::value_ptr(material.color));
        break;
    }
    case Selected::light:
    {
        Light& light = scene.lights[m_selected_id];
        bool dirty = ImGui::InputFloat3("Position", glm::value_ptr(light.local));
        dirty = dirty | ImGui::InputFloat3("Color", glm::value_ptr(light.color));
        if (dirty) {
            light.update(scene.entities[3].global_transform);
        }
        if (ImGui::Button("Remove")) {
            for (int i = m_selected_id; i + 1 < scene.lights.size(); i++) {
                std::swap(scene.lights[i], scene.lights[i + 1]);
            }
            scene.lights.pop_back();
            m_selected_id = std::min(m_selected_id, static_cast<int>(std::ssize(scene.lights) - 1));
        }
        if (ImGui::Button("Add new")) {
            auto& added = scene.lights.emplace_back(Light{
                .local = glm::vec3(),
                .global = glm::vec3(),
                .color = glm::vec3(1.0f, 1.0f, 1.0f),
            });
            added.update(scene.entities[3].global_transform);
        }
        break;
    }
    }
    ImGui::EndChild();

}

}