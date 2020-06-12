#include "input.h"
namespace vr
{

Input::Input(xr::Instance instance, xr::Session session)
{
    m_action_set = instance.createActionSet(xr::ActionSetCreateInfo{
        .actionSetName = "gameplay", 
        .localizedActionSetName = "Gameplay" });

    m_hand_subaction_paths = xr::BilateralPaths {
        instance.stringToPath("/user/hand/left"),
        instance.stringToPath("/user/hand/right")
    };

    m_select_action = m_action_set.createAction(xr::ActionCreateInfo{ 
        .actionName = "select", 
        .actionType = xr::ActionType::BooleanInput, 
        .countSubactionPaths = static_cast<uint32_t>(subaction_paths.size()),
        .subactionPaths = m_hand_subaction_paths.data(),
        .localizedActionName = "Select thing" });
    m_pose_action = m_action_set.createAction(xr::ActionCreateInfo{
        .actionName = "hand_pose",
        .actionType = xr::ActionType::PoseInput,
        .countSubactionPaths = static_cast<uint32_t>(subaction_paths.size()),
        .subactionPaths = m_hand_subaction_paths.data(),
        .localizedActionName = "Hand pose" });

    std::array bindings { 
        xr::ActionSuggestedBinding{ m_select_action, instance.stringToPath("/user/hand/left/input/x/click") },
        xr::ActionSuggestedBinding{ m_select_action, instance.stringToPath("/user/hand/right/input/a/click") },
        xr::ActionSuggestedBinding{ m_pose_action, instance.stringToPath("/user/hand/left/input/grip/pose") },
        xr::ActionSuggestedBinding{ m_pose_action, instance.stringToPath("/user/hand/right/input/grip/pose") }
    };
    instance.suggestInteractionProfileBindings(xr::InteractionProfileSuggestedBinding{
        .interactionProfile = instance.stringToPath("/interaction_profiles/oculus/touch_controller"),
        .countSuggestedBindings = static_cast<uint32_t>(bindings.size()),
        .suggestedBindings = bindings.data() });

    m_hand_space[0] = session.createActionSpace(xr::ActionSpaceCreateInfo{ .action = m_pose_action, .subactionPath = m_hand_subaction_paths[0] });
    m_hand_space[1] = session.createActionSpace(xr::ActionSpaceCreateInfo{ .action = m_pose_action, .subactionPath = m_hand_subaction_paths[1] });
    session.attachSessionActionSets(xr::SessionActionSetsAttachInfo{ .countActionSets = 1, .actionSets = &m_action_set });
    m_active_action_set = xr::ActiveActionSet{ .actionSet = m_action_set };
}

bool Input::sync_actions(xr::Session session)
{
    session.syncActions(xr::ActionsSyncInfo {
        .countActiveActionSets = 1u, 
        .activeActionSets = &m_active_action_set });

    bool pushed = false;
    for (size_t i = 0u; i < 2u; i++)
    {
        xr::ActionStateBoolean select_state = session.getActionStateBoolean(xr::ActionStateGetInfo{ m_select_action, m_hand_subaction_paths[i] });
        if (select_state.isActive) {
            pushed |= select_state.currentState;
            std::cout << "pushed" << std::endl;
        }
    }
    return pushed;
}

void Input::update_hand_poses(xr::Time display_time)
{
    for (size_t i = 0u; i < 2u; i++)
    {
        // Is it better to use baseSpace ?
        auto space_location = m_hand_space[i].locateSpace({}, display_time);
        xr::SpaceLocationFlags required_flags = 
            xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::PositionValid } | 
            xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::OrientationValid };
        if (space_location.locationFlags & required_flags) {
            last_known_hand_pose[i] = space_location.pose;
        }
    }
}

Input::~Input()
{
    if (m_pose_action) {
        m_pose_action.destroy();
    }
    if (m_select_action) {
        m_select_action.destroy();
    }
    if (m_action_set) {
        m_action_set.destroy();
    }
}

}