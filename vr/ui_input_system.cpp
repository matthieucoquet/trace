#include "ui_input_system.h"
#include "core/scene.h"
#include <fmt/core.h>

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

    std::array bindings { 
        xr::ActionSuggestedBinding{ m_select_action, instance.stringToPath("/user/hand/left/input/x/click") },
        xr::ActionSuggestedBinding{ m_select_action, instance.stringToPath("/user/hand/right/input/a/click") }
    };
    instance.suggestInteractionProfileBindings(xr::InteractionProfileSuggestedBinding{
        .interactionProfile = instance.stringToPath("/interaction_profiles/oculus/touch_controller"),
        .countSuggestedBindings = static_cast<uint32_t>(bindings.size()),
        .suggestedBindings = bindings.data() });

    action_sets.push_back(m_action_set);
    //session.attachSessionActionSets(xr::SessionActionSetsAttachInfo{ .countActionSets = 1, .actionSets = &m_action_set });
    m_active_action_set = xr::ActiveActionSet{ .actionSet = m_action_set };
}

void Ui_input_system::step(Scene& /*scene*/, xr::Session session, xr::Time /*display_time*/)
{
    session.syncActions(xr::ActionsSyncInfo {
        .countActiveActionSets = 1u, 
        .activeActionSets = &m_active_action_set });

    bool pushed = false;
    for (size_t i = 0u; i < 2u; i++)
    {
        xr::ActionStateBoolean select_state = session.getActionStateBoolean(xr::ActionStateGetInfo{
            .action = m_select_action.get(), 
            .subactionPath = m_hand_subaction_paths[i] });
        if (select_state.isActive) {
            pushed |= select_state.currentState == XR_TRUE;
            fmt::print("pushed\n");
        }
    }
}

}