#include "engine.h"
#include <iostream>
#include <limits>

void keyboard_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    auto engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
        engine->reload_shaders = true;
    }
}

Engine::Engine():
    m_window(this, keyboard_callback),
    m_vulkan_context(m_window/*, m_vr_context*/),
    m_scene(m_vulkan_context),
    m_renderer(m_vulkan_context, m_scene)
    //m_session(m_vr_context, m_vulkan_context),
{
}

Engine::~Engine()
{
    m_vulkan_context.device.waitIdle();
}

void Engine::run()
{
    //unsigned int count = 0u;
    while (m_window.step())
    {
        //Duration frame_time = Clock::now() - m_previous_clock;
        //std::cout << "Time : " << frame_time.count() << " - " << 1000 / frame_time.count() << " - " << count++ << std::endl;

        m_previous_clock = Clock::now();

        if (reload_shaders) {
            reset_renderer();
            reload_shaders = false;
        }
        else {
            m_renderer.step();
        }
    }
}

void Engine::reset_renderer()
{
    m_vulkan_context.device.waitIdle();
    m_renderer.reload_pipeline(m_vulkan_context);
}
