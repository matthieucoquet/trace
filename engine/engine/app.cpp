#include "app.hpp"

#include <fmt/core.h>
#include <limits>
#include <concepts>
#include <ranges>
#include <algorithm>
#include <imgui.h>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace sdf_editor
{

Vr_app::Vr_app(Scene scene, std::filesystem::path scene_json_path, std::filesystem::path scene_shader_path):
    m_scene(std::move(scene)),
    m_window(m_vr_instance.mirror_recommended_ratio()),
    m_context(m_window, &m_vr_instance)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    m_systems.push_back(std::make_unique<Input_glfw_system>(m_window.window));
    m_systems.push_back(std::make_unique<Json_system>(m_scene, std::move(scene_json_path)));
    m_systems.push_back(std::make_unique<Shader_system>(m_context, m_scene, scene_shader_path));
    m_systems.push_back(std::make_unique<Ui_system>());
    m_systems.push_back(std::make_unique<Transform_system>(m_scene));

    xr::GraphicsBindingVulkanKHR graphic_binding{
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

Vr_app::~Vr_app()
{
    m_context.device.waitIdle();
    std::ranges::for_each(m_systems, [this](auto& system) { system->cleanup(m_scene); });
    ImGui::DestroyContext();
}

void Vr_app::run()
{
    while (m_window.step())
    {
        Duration time_since_start = Clock::now() - m_start_clock;
        m_scene.scene_global.time = time_since_start.count();
        m_scene.scene_global.nb_lights = static_cast<int>(std::ssize(m_scene.lights));
        m_session->step(m_vr_instance.instance, m_scene, m_systems);
    }
}

static constexpr size_t size_command_buffers = 1u;

Desktop_app::Desktop_app(Scene scene, std::filesystem::path scene_json_path, std::filesystem::path scene_shader_path) :
    m_scene(std::move(scene)),
    m_json_system(m_scene, std::move(scene_json_path)),
    m_window(m_window_extent.width, m_window_extent.height),
    m_context(m_window, nullptr),
    m_shader_system(m_context, m_scene, std::move(scene_shader_path), true),
    m_transform_system(m_scene),
    m_renderer(m_context, m_scene, size_command_buffers),
    m_mirror(m_context, size_command_buffers, false),
    m_command_pools(m_context.device, m_context.queue_family, size_command_buffers)
{
    m_renderer.create_per_frame_data(m_context, m_scene, m_trace_extent, size_command_buffers);
    m_renderer.create_descriptor_sets(m_context.descriptor_pool, size_command_buffers);
}

Desktop_app::~Desktop_app()
{
    m_context.device.waitIdle();
    m_shader_system.cleanup(m_scene);
}

void Desktop_app::run()
{
    while (m_window.step())
    {
        Duration time_since_start = Clock::now() - m_start_clock;
        m_scene.scene_global.time = time_since_start.count();
        m_scene.scene_global.nb_lights = static_cast<int>(std::ssize(m_scene.lights));

        m_json_system.step(m_scene);
        m_shader_system.step(m_scene);
        m_ui_system.step(m_scene);
        m_transform_system.step(m_scene);

        for (size_t eye_id = 0u; eye_id < 2u; eye_id++)
        {
            m_scene.scene_global.eyes[eye_id].pose.position.x = m_scene.camera_position.x;
            m_scene.scene_global.eyes[eye_id].pose.position.y = m_scene.camera_position.y;
            m_scene.scene_global.eyes[eye_id].pose.position.z = m_scene.camera_position.z;
            //glm::quat rot = glm::angleAxis(m_scene.camera_rot_y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(m_scene.camera_rot_z, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::quat rot(glm::vec3(0.0, m_scene.camera_rot_y, 0.0));
            m_scene.scene_global.eyes[eye_id].pose.orientation = xr::Quaternionf{ .x = rot.x, .y = rot.y, .z = rot.z, .w = rot.w };
            float wfov = 1.04;
            float hfov = std::atan((std::tan(wfov) * m_trace_extent.height) / m_trace_extent.width);
            m_scene.scene_global.eyes[eye_id].fov.angleUp = hfov;
            m_scene.scene_global.eyes[eye_id].fov.angleDown = -hfov;
            m_scene.scene_global.eyes[eye_id].fov.angleRight = wfov;
            m_scene.scene_global.eyes[eye_id].fov.angleLeft = -wfov;
        }

        size_t command_pool_id = m_command_pools.find_next();
        auto& command_buffer = m_command_pools.command_buffers[command_pool_id];
        m_renderer.update_per_frame_data(m_scene, command_pool_id);

        m_renderer.start_recording(command_buffer, m_scene);
        m_renderer.trace(command_buffer, m_scene, command_pool_id, m_trace_extent);
        m_mirror.copy(command_buffer, m_renderer.per_frame[command_pool_id].storage_image.image, command_pool_id, m_trace_extent);
        m_renderer.end_recording(command_buffer, command_pool_id);
        command_buffer.end();
        m_mirror.present(command_buffer, m_command_pools.fences[command_pool_id], command_pool_id);
    }
}

}