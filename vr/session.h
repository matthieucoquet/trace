#pragma once

#include "scene.h"
#include "vr_common.h"
#include "vulkan/renderer.h"
#include "vr_swapchain.h"

namespace vulkan {
class Context;
}

namespace vr
{

class Instance;

class Session
{
public:
    xr::Session session;

    Session(Instance& instance, vulkan::Context& context, Scene& scene);
    Session(const Session& other) = delete;
    Session(Session&& other) = delete;
    Session& operator=(Session& other) = delete;
    Session& operator=(Session&& other) = delete;
    ~Session();

    void step(xr::Instance instance, Scene& scene);
private:
    xr::SystemId m_system_id;
    xr::Space m_stage_space;
    xr::SessionState m_session_state;
    
    Swapchain m_swapchain;

    vulkan::Renderer m_renderer;

    xr::CompositionLayerProjection composition_layer{};
    std::array<xr::CompositionLayerProjectionView, 2> composition_layer_views;

    void poll_events(xr::Instance instance);
    void draw_frame(Scene& scene);
    void handle_state_change(xr::EventDataSessionStateChanged& event_stage_changed);
};

}