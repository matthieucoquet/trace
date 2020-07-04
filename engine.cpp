#include "engine.h"
#include <iostream>
#include <limits>
#include <imgui.h>
constexpr bool vr_mode = true;

Engine::Engine() :
    m_window(m_vr_instance.mirror_recommended_ratio()),
    m_context(m_window, m_vr_instance),
    m_scene(m_context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto graphic_binding = xr::GraphicsBindingVulkanKHR{
        .instance = m_context.instance,
        .physicalDevice = m_context.physical_device,
        .device = m_context.device,
        .queueFamilyIndex = m_context.queue_family,
        .queueIndex = 0u
    };
    m_session.emplace(m_window.window,
        m_vr_instance.instance.createSession(xr::SessionCreateInfo{
        .next = xr::get(graphic_binding),
        .systemId = m_vr_instance.system_id
        }),
        m_vr_instance, m_context, m_scene);
}

Engine::~Engine()
{
    m_context.device.waitIdle();
}

void Engine::run()
{
    unsigned int count = 0u;
    while (m_window.step())
    {
        Duration frame_time = Clock::now() - m_previous_clock;
        std::cout << "Time : " << frame_time.count() << " - " << 1000 / frame_time.count() << " - " << count++ << std::endl;
        m_previous_clock = Clock::now();

        m_session->step(m_vr_instance.instance, m_scene);
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
