#include "ui_input_system.hpp"
#include "core/scene.hpp"
#include <imgui.h>

namespace vr
{

Ui_input_system::Ui_input_system(xr::Instance instance, xr::Session /*session*/, std::vector<xr::ActionSet>& action_sets)
{
    m_action_set = instance.createActionSet(xr::ActionSetCreateInfo{
        .actionSetName = "ui", 
        .localizedActionSetName = "UI" });

    m_hand_subaction_paths = xr::BilateralPaths {
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

void Ui_input_system::suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings)
{
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_select_action, instance.stringToPath("/user/hand/left/input/x/click") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_select_action, instance.stringToPath("/user/hand/right/input/a/click") });
}

void Ui_input_system::step(Scene& /*scene*/, xr::Session session, xr::Time /*display_time*/, xr::Space /*base_space*/, float /*offset_space_y*/)
{
    for (size_t i = 0u; i < 2u; i++)
    {
        xr::ActionStateBoolean select_state = session.getActionStateBoolean(xr::ActionStateGetInfo{
            .action = m_select_action.get(), 
            .subactionPath = m_hand_subaction_paths[i] });
        if (select_state.isActive) {
            if (select_state.currentState) {
                m_last_active_hand = i;
            }
        }
    }

    //const xr::Posef& pose = scene.last_known_hand_pose[m_last_active_hand];
    /*ImGuiIO& io = ImGui::GetIO();
    float updated_mouse_x = std::clamp(static_cast<float>(mouse_x), 0.0f, io.DisplaySize.x);
    float updated_mouse_y = std::clamp(static_cast<float>(mouse_y), 0.0f, io.DisplaySize.y);

    io.MousePos = ImVec2(updated_mouse_x, updated_mouse_y);*/
}

}