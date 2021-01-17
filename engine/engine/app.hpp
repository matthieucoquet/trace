#pragma once
#include "vulkan/vk_common.hpp"

#include "window.hpp"
#include "core/system.hpp"
#include "core/scene.hpp"
#include "vulkan/context.hpp"
#include "vulkan/renderer.hpp"
#include "vr/instance.hpp"
#include "vr/session.hpp"
#include "shader_system.hpp"
#include "ui_system.hpp"
#include "input_glfw_system.hpp"

#include <memory>
#include <optional>


class Vr_app
{
public:
    Vr_app(Scene scene, std::string_view scene_shader_path);
    Vr_app(const Vr_app& other) = delete;
    Vr_app(Vr_app&& other) = delete;
    Vr_app& operator=(const Vr_app& other) = delete;
    Vr_app& operator=(Vr_app&& other) = delete;
    ~Vr_app();

    void run();
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float>;

    Scene m_scene;
    vr::Instance m_vr_instance;
    Window m_window;
    vulkan::Context m_context;
    std::optional<vr::Session> m_session;  // When set, session is a valid session

    std::vector<std::unique_ptr<System>> m_systems;

    Time_point m_start_clock = Clock::now();
};

class Desktop_app
{
public:
    Desktop_app(Scene Desktop_app, std::string_view scene_shader_path);
    Desktop_app(const Desktop_app& other) = delete;
    Desktop_app(Desktop_app&& other) = delete;
    Desktop_app& operator=(const Desktop_app& other) = delete;
    Desktop_app& operator=(Desktop_app&& other) = delete;
    ~Desktop_app();

    void run();
private:
    using Clock = std::chrono::steady_clock;
    using Time_point = std::chrono::time_point<std::chrono::steady_clock>;
    using Duration = std::chrono::duration<float>;

    vk::Extent2D m_window_extent{ 1400, 900 };
    //vk::Extent2D m_trace_extent{ 2800, 1800 };
    vk::Extent2D m_trace_extent{ 11200, 7200 };
    Scene m_scene;
    Window m_window;
    vulkan::Context m_context;

    Time_point m_start_clock = Clock::now();

    Shader_system m_shader_system;

    vulkan::Renderer m_renderer;
    vulkan::Desktop_mirror m_mirror;
    vulkan::Reusable_command_pools m_command_pools;
};