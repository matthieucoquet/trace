#pragma once
#include "vr_common.hpp"
#include "input_system.hpp"
#include "core/scene.hpp"

namespace vr
{

class Main_input_system : public Input_system
{
public:
    Main_input_system(xr::Instance instance, xr::Session session, std::vector<xr::ActionSet>& action_sets);
    Main_input_system(const Main_input_system& other) = delete;
    Main_input_system(Main_input_system&& other) = delete;
    Main_input_system& operator=(const Main_input_system& other) = delete;
    Main_input_system& operator=(Main_input_system&& other) = delete;
    ~Main_input_system() override final = default;

    void suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings) override final;
    void step(Scene& scene, xr::Session session, xr::Time display_time, xr::Space base_space, float offset_space_y) override final;
private:
    xr::ActionSet m_action_set;
    xr::Action m_pose_action;
    xr::Action m_grab_action;
    xr::ActiveActionSet m_active_action_set;
    xr::BilateralPaths m_hand_subaction_paths;

    std::array<xr::Space, 2> m_hand_space;

    std::array<bool, 2> m_was_grabing;
    std::array<size_t, 2> m_grabed_id;
    std::array<glm::vec3, 2> m_diff_pos;
    std::array<glm::quat, 2> m_diff_rot;

};

}