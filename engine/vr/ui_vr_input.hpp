#pragma once
#include "vr_input.hpp"

namespace vr
{

class Ui_vr_input final : public Vr_input
{
public:
    Ui_vr_input(xr::Instance instance, xr::Session session, std::vector<xr::ActionSet>& action_sets);
    Ui_vr_input(const Ui_vr_input& other) = delete;
    Ui_vr_input(Ui_vr_input&& other) = delete;
    Ui_vr_input& operator=(const Ui_vr_input& other) = delete;
    Ui_vr_input& operator=(Ui_vr_input&& other) = delete;
    ~Ui_vr_input() override = default;

    void suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings) override final;
    void step(Scene& scene, xr::Session session, xr::Time display_time, xr::Space base_space, float offset_space_y) override final;
private:
    xr::ActionSet m_action_set;
    xr::Action m_select_action;
    xr::ActiveActionSet m_active_action_set;
    std::array<xr::Path, 2> m_hand_subaction_paths;

    size_t m_last_active_hand{};
};

}
