#pragma once

#include "vr_common.h"

namespace vr
{

class Input
{
public:
    std::array<xr::Posef, 2> last_known_hand_pose;

    Input(xr::Instance instance, xr::Session sessio);
    Input(const Input& other) = delete;
    Input(Input&& other) = delete;
    Input& operator=(const Input& other) = delete;
    Input& operator=(Input&& other) = delete;
    ~Input();

    bool sync_actions(xr::Session session);
    void update_hand_poses(xr::Time display_time);
private:
    xr::ActionSet m_action_set;
    xr::Action m_select_action;
    xr::Action m_pose_action;
    xr::ActiveActionSet m_active_action_set;
    xr::BilateralPaths m_hand_subaction_paths;

    std::array<xr::Space, 2> m_hand_space;
};

}