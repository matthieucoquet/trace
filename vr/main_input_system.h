#pragma once
#include "vr_common.h"
#include "input_system.h"

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

    virtual void step(Scene& scene, xr::Session session, xr::Time display_time) override final;
private:
    xr::ActionSet m_action_set;
    xr::Action m_pose_action;
    xr::ActiveActionSet m_active_action_set;
    xr::BilateralPaths m_hand_subaction_paths;

    std::array<xr::Space, 2> m_hand_space;
};

}