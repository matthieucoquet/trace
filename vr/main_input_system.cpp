#include "main_input_system.hpp"
#include "core/scene.hpp"
#include "core/transform_helpers.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fmt/core.h>

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
        .actionType = xr::ActionType::BooleanInput,
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
            auto& prim = scene.primitives[i];
            prim.position.x = space_location.pose.position.x;
            prim.position.y = space_location.pose.position.y;
            prim.position.z = space_location.pose.position.z;
            prim.rotation.w = space_location.pose.orientation.w;
            prim.rotation.x = space_location.pose.orientation.x;
            prim.rotation.y = space_location.pose.orientation.y;
            prim.rotation.z = space_location.pose.orientation.z;
            glm::mat4 model_to_world = glm::translate(prim.position) * glm::toMat4(prim.rotation) * glm::scale(glm::vec3(prim.scale));
            scene.primitive_transform[i] = glm::inverse(model_to_world);
        }

        xr::ActionStateBoolean grab_state = session.getActionStateBoolean(xr::ActionStateGetInfo{
           .action = m_grab_action.get(),
           .subactionPath = m_hand_subaction_paths[i] });

        // TODO extract all that math to an helpers
        if (grab_state.isActive && grab_state.currentState) {
            const auto& hand = scene.primitives[i];
            if (m_was_grabing[i])
            {
                if (m_grabed_ui[i]) {
                    Primitive& prim = scene.ui_primitive;
                    auto [pos, rot] = transform(hand.position, hand.rotation, m_diff_pos[i], m_diff_rot[i]);
                    prim.position = pos;
                    prim.rotation = rot;
                    scene.scene_global.ui_position = pos;
                    scene.scene_global.ui_normal = glm::rotate(rot, glm::vec3(0.0f, 0.0f, 1.0f));
                }
                else {
                    Primitive& prim = scene.primitives[m_grabed_id[i]];
                    auto [pos, rot] = transform(hand.position, hand.rotation, m_diff_pos[i], m_diff_rot[i]);
                    prim.position = pos;
                    prim.rotation = rot;
                    glm::mat4 model_to_world = glm::translate(prim.position) * glm::toMat4(prim.rotation) * glm::scale(glm::vec3(prim.scale));
                    scene.primitive_transform[m_grabed_id[i]] = glm::inverse(model_to_world);
                }
            }
            else
            {
                // Check UI first
                Primitive& ui = scene.ui_primitive;
                glm::vec3 ui_normal = glm::rotate(ui.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
                float z_dist = std::abs(glm::dot(ui_normal, ui.position - hand.position));
                m_grabed_ui[i] = z_dist < 0.1f && glm::length2(ui.position - hand.position) <= (0.25 * ui.scale * ui.scale);
                if (m_grabed_ui[i]) {
                    auto [pos, rot] = compute_local_transform(hand.position, hand.rotation, ui.position, ui.rotation);
                    m_diff_rot[i] = rot;
                    m_diff_pos[i] = pos;
                    m_was_grabing[i] = true;
                }
                else {
                    for (size_t p_id = 2u; p_id < scene.primitives.size(); p_id++) {
                        auto& prim = scene.primitives[p_id];
                        if (glm::length2(prim.position - hand.position) <= (0.25 * prim.scale * prim.scale)) {
                            m_grabed_id[i] = p_id;
                            auto [pos, rot] = compute_local_transform(hand.position, hand.rotation, prim.position, prim.rotation);
                            m_diff_rot[i] = rot;
                            m_diff_pos[i] = pos;
                            m_was_grabing[i] = true;
                            break;
                        }
                    }
                }
            }
        } 
        else {
            m_was_grabing[i] = false;
        }
    }
}

}