#pragma once

#include "vr_common.h"
#include "input_system.h"
#include "vr_swapchain.h"
#include "core/system.h"
#include "core/scene.h"
#include "vulkan/renderer.h"
#include "vulkan/desktop_mirror.h"
#include "vulkan/command_buffer.h"
#include "vulkan/imgui_render.h"
#include <memory>

struct GLFWwindow;
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

    Swapchain m_main_swapchain;
    Swapchain m_ui_swapchain;

    vulkan::Renderer m_renderer;
    vulkan::Desktop_mirror m_mirror;
    vulkan::Reusable_command_buffers m_command_buffers;
    vulkan::Imgui_render m_imgui_render;

    xr::CompositionLayerProjection composition_layer_proj{};
    std::array<xr::CompositionLayerProjectionView, 2> composition_layer_views;
    xr::CompositionLayerQuad composition_layer_ui{};

    std::vector<std::unique_ptr<Input_system>> m_input_systems;
    std::vector<xr::ActionSet> m_action_sets;
    std::vector<xr::ActiveActionSet> m_active_action_sets;

    void poll_events(xr::Instance instance);
    void draw_frame(Scene& scene, std::vector<std::unique_ptr<System>>& systems);
    void handle_state_change(xr::EventDataSessionStateChanged& event_stage_changed);
};

}