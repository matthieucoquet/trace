#pragma once
#include "vulkan/vk_common.hpp"

#include "window.hpp"
#include "core/system.hpp"
#include "core/scene.hpp"
#include "vulkan/context.hpp"
#include "vulkan/renderer.hpp"
#include "vr/instance.hpp"
#include "vr/session.hpp"

#include <marl/scheduler.h>
#include <memory>
#include <optional>


class Engine
{
public:
    Engine(Scene scene, std::string_view scene_shader_path);
    Engine(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine& operator=(Engine&& other) = delete;
    ~Engine();

    void run();
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float>;

    Scene m_scene;
    marl::Scheduler m_scheduler{ marl::Scheduler::Config::allCores() };
    vr::Instance m_vr_instance;
    Window m_window;
    vulkan::Context m_context;    
    std::optional<vr::Session> m_session;  // When set, session is a valid session

    std::vector<std::unique_ptr<System>> m_systems;

    Time_point m_start_clock = Clock::now();
};

