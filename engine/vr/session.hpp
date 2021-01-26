#pragma once

#include "vr_common.hpp"
#include "vr_input.hpp"
#include "vr_swapchain.hpp"
#include "core/system.hpp"
#include "core/scene.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/desktop_mirror.hpp"
#include "vulkan/command_buffer.hpp"
#include <memory>

struct GLFWwindow;
namespace sdf_editor::vulkan {
class Context;
}

namespace sdf_editor::vr
{

class Instance;

class Session
{
public:
    xr::Session session;

    Session(xr::Session new_session, Instance& instance, vulkan::Context& context, Scene& scene);
    Session(const Session& other) = delete;
    Session(Session&& other) = delete;
    Session& operator=(Session& other) = delete;
    Session& operator=(Session&& other) = delete;
    ~Session();

    void step(xr::Instance instance, Scene& scene, std::vector<std::unique_ptr<System>>& systems);
private:
    xr::SystemId m_system_id;
    xr::Space m_stage_space;
    xr::SessionState m_session_state;

    Swapchain m_swapchain;

    vulkan::Renderer m_renderer;
    vulkan::Desktop_mirror m_mirror;
    vulkan::Reusable_command_pools m_command_pools;

    xr::CompositionLayerProjection composition_layer{};
    std::array<xr::CompositionLayerProjectionView, 2> composition_layer_views;

    std::vector<std::unique_ptr<Vr_input>> m_input_systems;
    std::vector<xr::ActionSet> m_action_sets;
    std::vector<xr::ActiveActionSet> m_active_action_sets;

    void poll_events(xr::Instance instance);
    void draw_frame(Scene& scene, std::vector<std::unique_ptr<System>>& systems);
    void handle_state_change(xr::EventDataSessionStateChanged& event_stage_changed);
};

}