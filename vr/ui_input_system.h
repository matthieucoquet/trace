#pragma once
#include "vr_common.h"
#include "input_system.h"

namespace vr
{

class Ui_input_system : public Input_system
{
public:
    Ui_input_system(xr::Instance instance, xr::Session session);
    Ui_input_system(const Ui_input_system& other) = delete;
    Ui_input_system(Ui_input_system&& other) = delete;
    Ui_input_system& operator=(const Ui_input_system& other) = delete;
    Ui_input_system& operator=(Ui_input_system&& other) = delete;
    ~Ui_input_system() override final = default;

    virtual void step(Scene& scene, xr::Session session, xr::Time display_time) override final;
private:
    xr::ActionSet m_action_set;
    xr::Action m_select_action;
    xr::ActiveActionSet m_active_action_set;
    xr::BilateralPaths m_hand_subaction_paths;
};

}