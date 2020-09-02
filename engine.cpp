#include "engine.h"
#include "shader_system.h"
#include "ui_system.h"
#include "input_glfw_system.h"
#include <fmt/core.h>
#include <limits>
#include <concepts>
#include <ranges>
#include <algorithm>
#include <imgui.h>
constexpr bool vr_mode = true;

Engine::Engine() :
    m_window(m_vr_instance.mirror_recommended_ratio()),
    m_context(m_window, m_vr_instance),
    m_scene()
{
    m_scheduler.bind();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    m_systems.push_back(std::make_unique<Input_glfw_system>(m_window.window));
    m_systems.push_back(std::make_unique<Shader_system>(m_context, m_scene));
    m_systems.push_back(std::make_unique<Ui_system>());

    auto graphic_binding = xr::GraphicsBindingVulkanKHR{
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
        m_vr_instance, m_context, m_scene);

}

Engine::~Engine()
{
    auto checkpoints_data = m_context.graphics_queue.getCheckpointDataNV();
    m_context.device.waitIdle();
    std::ranges::for_each(m_systems, [this](auto& system) { system->cleanup(m_scene); });
    ImGui::DestroyContext();
    m_scheduler.unbind();
}

void Engine::run()
{
    //unsigned int count = 0u;
    while (m_window.step())
    {
        //Duration frame_time = Clock::now() - m_previous_clock;
        //fmt::core("Time : {} - {} - {}\n", frame_time.count(), 1000 / frame_time.count(),  count++);
        //m_previous_clock = Clock::now();

        m_session->step(m_vr_instance.instance, m_scene, m_systems);
        m_scene.step();
        if (reload_shaders) {
            reset_renderer();
            reload_shaders = false;
        }
        else {
            //m_renderer.step(m_scene);
        }
    }
}

void Engine::reset_renderer()
{
    m_context.device.waitIdle();
    //m_renderer.reload_pipeline(m_vulkan_context);
}
