#pragma once
#include "vr_common.h"
#include "input_system.h"

namespace vr
{

class Ui_input_system : public Input_system
{
public:
    Ui_input_system(xr::Instance instance, xr::Session session, std::vector<xr::ActionSet>& action_sets);
    Ui_input_system(const Ui_input_system& other) = delete;
    Ui_input_system(Ui_input_system&& other) = delete;
    Ui_input_system& operator=(const Ui_input_system& other) = delete;
    Ui_input_system& operator=(Ui_input_system&& other) = delete;
    ~Ui_input_system() override final = default;

    void suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings) override final;
    void step(Scene& scene, xr::Session session, xr::Time display_time, xr::Space base_space, float offset_space_y) override final;
private:
    xr::ActionSet m_action_set;
    xr::Action m_select_action;
    xr::ActiveActionSet m_active_action_set;
    xr::BilateralPaths m_hand_subaction_paths;

    size_t m_last_active_hand{};
};

}