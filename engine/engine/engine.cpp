#include "engine.hpp"
#include "shader_system.hpp"
#include "ui_system.hpp"
#include "input_glfw_system.hpp"

#include <fmt/core.h>
#include <limits>
#include <concepts>
#include <ranges>
#include <algorithm>
#include <imgui.h>

Engine::Engine(Scene scene, std::string_view scene_shader_path) :
    m_scene(std::move(scene)),
    m_window(m_vr_instance.mirror_recommended_ratio()),
    m_context(m_window, m_vr_instance)
{
    m_scheduler.bind();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    m_systems.push_back(std::make_unique<Input_glfw_system>(m_window.window));
    m_systems.push_back(std::make_unique<Shader_system>(m_context, m_scene, scene_shader_path));
    m_systems.push_back(std::make_unique<Ui_system>());

    xr::GraphicsBindingVulkanKHR graphic_binding {
        .instance = m_context.instance,
        .physicalDevice = m_context.physical_device,
        .device = m_context.device,
        .queueFamilyIndex = m_context.queue_family,
        .queueIndex = 0u
    };
    m_session.emplace(
        m_vr_instance.instance.createSession(xr::SessionCreateInfo{
                .next = xr::get(graphic_binding),
                .systemId = m_vr_instance.system_id
            }),
            m_vr_instance, m_context, m_scene
    );
}

Engine::~Engine()
{
    m_context.device.waitIdle();
    std::ranges::for_each(m_systems, [this](auto& system) { system->cleanup(m_scene); });
    ImGui::DestroyContext();
    m_scheduler.unbind();
}

void Engine::run()
{
    while (m_window.step())
    {
        m_session->step(m_vr_instance.instance, m_scene, m_systems);
        Duration time_since_start = Clock::now() - m_start_clock;
        m_scene.scene_global.time = time_since_start.count();
    }
}

