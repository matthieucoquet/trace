#pragma once
#include "vulkan/vk_common.h"

#include "window.h"
#include "core/system.h"
#include "core/scene.h"
#include "vulkan/context.h"
#include "vulkan/renderer.h"
#include "vr/instance.h"
#include "vr/session.h"

#include <marl/scheduler.h>
#include <memory>
#include <optional>


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

    marl::Scheduler m_scheduler{ marl::Scheduler::Config::allCores() };
    vr::Instance m_vr_instance;
    Window m_window;
    vulkan::Context m_context;
    Scene m_scene;
    std::optional<vr::Session> m_session;  // When set, session is a valid session

    std::vector<std::unique_ptr<System>> m_systems;

    Time_point m_previous_clock;

    void reset_renderer();
};

