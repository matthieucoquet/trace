#include "ui_vr_input.hpp"
#include "core/scene.hpp"
#include <imgui.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include "core/transform_helpers.hpp"

namespace vr
{

Ui_vr_input::Ui_vr_input(xr::Instance instance, xr::Session /*session*/, std::vector<xr::ActionSet>& action_sets)
{
    m_action_set = instance.createActionSet(xr::ActionSetCreateInfo{
        .actionSetName = "ui", 
        .localizedActionSetName = "UI" });

    m_hand_subaction_paths = std::array<xr::Path, 2> {
        instance.stringToPath("/user/hand/left"),
        instance.stringToPath("/user/hand/right")
    };

    m_select_action = m_action_set.createAction(xr::ActionCreateInfo{ 
        .actionName = "select", 
        .actionType = xr::ActionType::BooleanInput, 
        .countSubactionPaths = static_cast<uint32_t>(m_hand_subaction_paths.size()),
        .subactionPaths = m_hand_subaction_paths.data(),
        .localizedActionName = "Select thing" });

    action_sets.push_back(m_action_set);
}

void Ui_vr_input::suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings)
{
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_select_action, instance.stringToPath("/user/hand/left/input/x/click") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_select_action, instance.stringToPath("/user/hand/right/input/a/click") });
}

void Ui_vr_input::step(Scene& scene, xr::Session session, xr::Time /*display_time*/, xr::Space /*base_space*/, float /*offset_space_y*/)
{
    for (size_t i = 0u; i < 2u; i++)
    {
        xr::ActionStateBoolean select_state = session.getActionStateBoolean(xr::ActionStateGetInfo{
            .action = m_select_action.get(), 
            .subactionPath = m_hand_subaction_paths[i] });
        if (select_state.isActive) {
            bool pressed = bool(select_state.currentState);
            if (pressed) {
                scene.mouse_control = false;
                m_last_active_hand = i;
            }
            if (!scene.mouse_control && m_last_active_hand == i) {
                ImGuiIO& io = ImGui::GetIO();
                io.MouseDown[ImGuiMouseButton_Left] = pressed;
            }
        }
    }

    if (!scene.mouse_control)
    {
        Object& ui = scene.ui_object;
        Object& hand = scene.objects[m_last_active_hand];

        auto [pos, rot] = compute_local_transform(ui.position, ui.rotation, hand.position, hand.rotation);
        glm::vec3 ptr_direction = glm::rotate(rot, glm::vec3(0.0f, 0.0f, 1.0f));
        float t = -pos.z / ptr_direction.z;
        glm::vec3 mouse = pos + t * ptr_direction;
        mouse = (mouse + 0.5f * ui.scale) / ui.scale;

        if (mouse.x >= 0.0f && mouse.x <= 1.0f && mouse.y >= 0.0f && mouse.y <= 1.0f)
        {
            ImGuiIO& io = ImGui::GetIO();
            io.MousePos = ImVec2(mouse.x * io.DisplaySize.x, (1.0f - mouse.y) * io.DisplaySize.y);
        }
    }
}

}