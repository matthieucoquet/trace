#include "scene_vr_input.hpp"
#include "glm_helpers.hpp"
#include "core/scene.hpp"
#include "core/transform.hpp"

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

void Scene_vr_input::step(Scene& scene, xr::Session session, xr::Time display_time, xr::Space base_space, float /*offset_space_y*/)
{
    for (int i = 0; i < 2; i++)
    {
        auto space_location = m_hand_space[i].locateSpace(base_space, display_time);
        xr::SpaceLocationFlags required_flags = 
            xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::PositionValid } | 
            xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::OrientationValid };
        if ((space_location.locationFlags & required_flags) == required_flags) {
            //space_location.pose.position.y += offset_space_y;
            auto& entity = scene.entities[i];
            to_glm(space_location.pose, entity);
        }

        xr::ActionStateBoolean grab_state = session.getActionStateBoolean(xr::ActionStateGetInfo{
           .action = m_grab_action.get(),
           .subactionPath = m_hand_subaction_paths[i] });
        xr::ActionStateFloat scale_state = session.getActionStateFloat(xr::ActionStateGetInfo{
           .action = m_scale_action.get(),
           .subactionPath = m_hand_subaction_paths[i] });

        float scale = (scale_state.isActive && std::abs(scale_state.currentState) > 0.3) ? (1.0f + 0.02f * scale_state.currentState) : 1.0f;

        if (grab_state.isActive && grab_state.currentState) {
            const auto& hand = scene.entities[i];
            if (m_was_grabing[i])
            {
                for (size_t p_id = 2u; p_id < scene.entities.size(); p_id++) {
                    Entity& entity = scene.entities[p_id];
                    entity.visit([this, &scene, &hand, scale, i](Entity& entity) {
                        if (entity.hand_grabbing == i) {
                            auto global_scale = entity.global_transform.scale;
                            if (entity.group_id == Entity::scene_id) {
                                entity.global_transform = Transform{ .position = hand.global_transform.position } * m_diff[i];
                                for (auto& light : scene.lights) {
                                    light.update(entity.global_transform);
                                }
                            }
                            else {
                                entity.global_transform = hand.global_transform * m_diff[i];
                            }
                            entity.global_transform.scale = std::clamp(scale * global_scale, 0.03f, 10.0f);
                            entity.dirty_local = true;
                        }
                        });
                }
            }
            else
            {
                Entity* candidate = nullptr; // To make sure only one is selected
                for (size_t p_id = 2u; p_id < scene.entities.size(); p_id++) {
                    auto& entity = scene.entities[p_id];
                    entity.visit([this, &hand, &candidate](Entity& entity) {
                        entity.hand_grabbing = -1;
                        if (entity.group_id >= Entity::empty_id) {
                            if (!candidate && entity.group_id == Entity::scene_id) {
                                candidate = &entity;
                            }
                        }
                        else {
                            if (glm::length2(entity.global_transform.position - hand.global_transform.position) <= (0.25 * entity.global_transform.scale * entity.global_transform.scale)) {
                                candidate = &entity;
                            }
                        }
                        });
                }
                if (candidate) {
                    candidate->hand_grabbing = i;
                    if (candidate->group_id == Entity::scene_id) {
                        m_diff[i] = Transform{ .position = hand.global_transform.position }.inverse() * candidate->global_transform;
                    }
                    else {
                        m_diff[i] = hand.global_transform.inverse() * candidate->global_transform;
                    }
                    m_was_grabing[i] = true;
                }
            }
        } 
        else {
            m_was_grabing[i] = false;
        }
    }
}

}