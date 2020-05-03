#include "engine.h"
#include <iostream>
#include <limits>

Engine::Engine():
    m_window(),
    m_context(m_window),
    m_scene(m_context),
    m_renderer(m_context, m_scene)
{
}

Engine::~Engine()
{
    m_context.device.waitIdle();
}

void Engine::run()
{
    while (m_window.step())
    {
        /*if (m_window.framebuffer_resize) {
            reset_renderer();
        }
        else {*/
            m_renderer.step();
        //}
    }
}

/*void Engine::reset_renderer()
{
    m_device().waitIdle();
    if (!m_window.framebuffer_minimized)
    {
        m_renderer.destroy(m_device);
        m_renderer = Renderer(m_device, m_scene);
        m_window.framebuffer_resize = false;
    }
}*/
