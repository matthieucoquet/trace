#pragma once
#include "vulkan/common.h"

#include "window.h"
#include "scene.h"
#include "vulkan/context.h"
#include "vulkan/renderer.h"
#include "vr/context.h"
#include "vr/session.h"

class Engine
{
public:
    Engine();
    Engine(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine& operator=(Engine&& other) = delete;
    ~Engine();

    void run();

    bool reload_shaders = false;
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float, std::milli>;

    Window m_window;
    //vr::Context m_vr_context;
    vulkan::Context m_vulkan_context;
    Scene m_scene;
    vulkan::Renderer m_renderer;
    //vr::Session m_session;

    Time_point m_previous_clock;

    void reset_renderer();
};
