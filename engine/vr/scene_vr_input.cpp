#include "scene_vr_input.hpp"
#include "glm_helpers.hpp"
#include "core/scene.hpp"
#include "core/transform_helpers.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sdf_editor::vr
{

Scene_vr_input::Scene_vr_input(xr::Instance instance, xr::Session session, std::vector<xr::ActionSet>& action_sets)
{
    m_action_set = instance.createActionSet(xr::ActionSetCreateInfo{
        .actionSetName = "control", 
        .localizedActionSetName = "Control" });

    m_hand_subaction_paths = std::array<xr::Path, 2> {
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

    m_scale_action = m_action_set.createAction(xr::ActionCreateInfo{
        .actionName = "scale",
        .actionType = xr::ActionType::FloatInput,
        .countSubactionPaths = static_cast<uint32_t>(m_hand_subaction_paths.size()),
        .subactionPaths = m_hand_subaction_paths.data(),
        .localizedActionName = "Scale object" });

    m_hand_space[0] = session.createActionSpace(xr::ActionSpaceCreateInfo{
        .action = m_pose_action,
        .subactionPath = m_hand_subaction_paths[0],
        .poseInActionSpace = {.orientation = {0.0f, 0.0f, 0.0f, 1.0f} , .position = {0.0f, 0.0f, 0.0f} } });
    m_hand_space[1] = session.createActionSpace(xr::ActionSpaceCreateInfo{
        .action = m_pose_action, .subactionPath = m_hand_subaction_paths[1],
        .poseInActionSpace = {.orientation = {0.0f, 0.0f, 0.0f, 1.0f} , .position = {0.0f, 0.0f, 0.0f} } });
    action_sets.push_back(m_action_set);
}

void Scene_vr_input::suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings)
{
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_pose_action, instance.stringToPath("/user/hand/left/input/aim/pose") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_pose_action, instance.stringToPath("/user/hand/right/input/aim/pose") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_grab_action, instance.stringToPath("/user/hand/left/input/squeeze/value") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_grab_action, instance.stringToPath("/user/hand/right/input/squeeze/value") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_scale_action, instance.stringToPath("/user/hand/left/input/thumbstick/y") });
    suggested_bindings.suggested_binding_oculus.push_back(xr::ActionSuggestedBinding{ m_scale_action, instance.stringToPath("/user/hand/right/input/thumbstick/y") });
}

void Scene_vr_input::step(Scene& scene, xr::Session session, xr::Time display_time, xr::Space base_space, float offset_space_y)
{
    for (size_t i = 0u; i < 2u; i++)
    {
        auto space_location = m_hand_space[i].locateSpace(base_space, display_time);
        xr::SpaceLocationFlags required_flags = 
            xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::PositionValid } | 
            xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::OrientationValid };
        if ((space_location.locationFlags & required_flags) == required_flags) {
            space_location.pose.position.y += offset_space_y;
            auto& obj = scene.objects[i];
            to_glm(space_location.pose, obj);
            glm::mat4 model_to_world = glm::translate(obj.position) * glm::toMat4(obj.rotation) * glm::scale(glm::vec3(obj.scale));
            scene.objects_transform[i] = glm::inverse(model_to_world);
        }

        xr::ActionStateBoolean grab_state = session.getActionStateBoolean(xr::ActionStateGetInfo{
           .action = m_grab_action.get(),
           .subactionPath = m_hand_subaction_paths[i] });
        xr::ActionStateFloat scale_state = session.getActionStateFloat(xr::ActionStateGetInfo{
           .action = m_scale_action.get(),
           .subactionPath = m_hand_subaction_paths[i] });

        float scale = (scale_state.isActive && std::abs(scale_state.currentState) > 0.3) ? (1.0f + 0.02f * scale_state.currentState) : 1.0f;

        if (grab_state.isActive && grab_state.currentState) {
            const auto& hand = scene.objects[i];
            if (m_was_grabing[i])
            {
                if (m_grabed_ui[i]) {
                    Object& obj = scene.ui_object;
                    auto [pos, rot] = transform(hand.position, hand.rotation, m_diff_pos[i], m_diff_rot[i]);
                    obj.position = pos;
                    obj.rotation = rot;
                    obj.scale = std::clamp(scale * obj.scale, 0.5f, 5.0f);
                    scene.scene_global.ui_position = pos;
                    scene.scene_global.ui_normal = glm::rotate(rot, glm::vec3(0.0f, 0.0f, 1.0f));
                }
                else {
                    Object& obj = scene.objects[m_grabed_id[i]];
                    auto [pos, rot] = transform(hand.position, hand.rotation, m_diff_pos[i], m_diff_rot[i]);
                    obj.position = pos;
                    obj.rotation = rot;
                    obj.scale = std::clamp(scale * obj.scale, 0.03f, 5.0f);
                    glm::mat4 model_to_world = glm::translate(obj.position) * glm::toMat4(obj.rotation) * glm::scale(glm::vec3(obj.scale));
                    scene.objects_transform[m_grabed_id[i]] = glm::inverse(model_to_world);
                }
            }
            else
            {
                // Check UI first
                Object& ui = scene.ui_object;
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
                    for (size_t p_id = 2u; p_id < scene.objects.size(); p_id++) {
                        auto& obj = scene.objects[p_id];
                        if (glm::length2(obj.position - hand.position) <= (0.25 * obj.scale * obj.scale)) {
                            m_grabed_id[i] = p_id;
                            auto [pos, rot] = compute_local_transform(hand.position, hand.rotation, obj.position, obj.rotation);
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