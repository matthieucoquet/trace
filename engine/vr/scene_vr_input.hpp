#pragma once
#include "vr_common.hpp"
#include "vr_input.hpp"
#include "core/scene.hpp"

namespace vr
{

class Scene_vr_input : public Vr_input
{
public:
    Scene_vr_input(xr::Instance instance, xr::Session session, std::vector<xr::ActionSet>& action_sets);
    Scene_vr_input(const Scene_vr_input& other) = delete;
    Scene_vr_input(Scene_vr_input&& other) = delete;
    Scene_vr_input& operator=(const Scene_vr_input& other) = delete;
    Scene_vr_input& operator=(Scene_vr_input&& other) = delete;
    ~Scene_vr_input() override final = default;

    void suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings) override final;
    void step(Scene& scene, xr::Session session, xr::Time display_time, xr::Space base_space, float offset_space_y) override final;
private:
    xr::ActionSet m_action_set;
    xr::Action m_pose_action;
    xr::Action m_grab_action;
    xr::Action m_scale_action;
    xr::ActiveActionSet m_active_action_set;
    xr::BilateralPaths m_hand_subaction_paths;

    std::array<xr::Space, 2> m_hand_space;

    std::array<bool, 2> m_was_grabing;

    std::array<bool, 2> m_grabed_ui;
    std::array<size_t, 2> m_grabed_id;
    std::array<glm::vec3, 2> m_diff_pos;
    std::array<glm::quat, 2> m_diff_rot;

};

}