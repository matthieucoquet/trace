#include "main_input_system.hpp"
#include "core/scene.hpp"

namespace vr
{

Main_input_system::Main_input_system(xr::Instance instance, xr::Session session, std::vector<xr::ActionSet>& action_sets)
{
    m_action_set = instance.createActionSet(xr::ActionSetCreateInfo{
        .actionSetName = "control", 
        .localizedActionSetName = "Control" });

    m_hand_subaction_paths = xr::BilateralPaths {
        instance.stringToPath("/user/hand/left"),
        instance.stringToPath("/user/hand/right")
    };

    m_pose_action = m_action_set.createAction(xr::ActionCreateInfo{
        .actionName = "hand_pose",
        .actionType = xr::ActionType::PoseInput,
        .countSubactionPaths = static_cast<uint32_t>(m_hand_subaction_paths.size()),
        .subactionPaths = m_hand_subaction_paths.data(),
        .localizedActionName = "Hand pose" });

    m_grab_action = m_action_set.createAction(xr::ActionCreateInfo{
        .actionName = "grab",
        .actionType = xr::ActionType::FloatInput,
        .countSubactionPaths = static_cast<uint32_t>(m_hand_subaction_paths.size()),
        .subactionPaths = m_hand_subaction_paths.data(),
        .localizedActionName = "Grab object" });

    m_hand_space[0] = session.createActionSpace(xr::ActionSpaceCreateInfo{ .action = m_pose_action, .subactionPath = m_hand_subaction_paths[0] });
    m_hand_space[1] = session.createActionSpace(xr::ActionSpaceCreateInfo{ .action = m_pose_action, .subactionPath = m_hand_subaction_paths[1] });
    action_sets.push_back(m_action_set);
}

void Main_input_system::suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings)
{
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_pose_action, instance.stringToPath("/user/hand/left/input/grip/pose") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_pose_action, instance.stringToPath("/user/hand/right/input/grip/pose") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_grab_action, instance.stringToPath("/user/hand/left/input/squeeze/value") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_grab_action, instance.stringToPath("/user/hand/right/input/squeeze/value") });
}

void Main_input_system::step(Scene& scene, xr::Session session, xr::Time display_time, xr::Space base_space, float offset_space_y)
{
    for (size_t i = 0u; i < 2u; i++)
    {
        /*auto poseState =*/ session.getActionStatePose(xr::ActionStateGetInfo{ .action = m_pose_action, .subactionPath = m_hand_subaction_paths[0] });
        //renderHand[hand] = poseState.isActive;

        auto space_location = m_hand_space[i].locateSpace(base_space, display_time);
        xr::SpaceLocationFlags required_flags = 
            xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::PositionValid } | 
            xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::OrientationValid };
        if ((space_location.locationFlags & required_flags) == required_flags) {
            space_location.pose.position.y += offset_space_y;
            scene.last_known_hand_pose[i] = space_location.pose;
        }

        xr::ActionStateFloat select_state = session.getActionStateFloat(xr::ActionStateGetInfo{
        .action = m_grab_action.get(),
        .subactionPath = m_hand_subaction_paths[i] });
        if (select_state.isActive) {
        }
    }


}

}