#include "session.hpp"
#include "instance.hpp"
#include "glm_helpers.hpp"
#include "vulkan/context.hpp"
#include "scene_vr_input.hpp"
#include "ui_vr_input.hpp"
#include <marl/scheduler.h>
#include <fmt/core.h>
#include <vector>

namespace sdf_editor::vr
{
constexpr bool verbose = false;
constexpr size_t size_command_buffers = 4u;

Session::Session(xr::Session new_session, Instance& instance, vulkan::Context& context, Scene& scene) :
    session(new_session),
    m_swapchain(instance, session, context),
    //m_ui_swapchain(session, context, xr::Extent2Di{ .width = 1000, .height = 1000 }),
    m_renderer(context, scene, size_command_buffers),
    m_mirror(context, size_command_buffers),
    m_command_pools(context.device, context.queue_family, size_command_buffers)
{
    if constexpr (verbose) {
        auto reference_spaces = session.enumerateReferenceSpacesToVector();
        fmt::print("XR runtime supported reference spaces:\n");
        for (auto reference_space : reference_spaces) {
            fmt::print("\t{}\n", xr::to_string_literal(reference_space));
        }
    }
    m_stage_space = session.createReferenceSpace(xr::ReferenceSpaceCreateInfo{
        .referenceSpaceType = Scene::standing ? xr::ReferenceSpaceType::Stage : xr::ReferenceSpaceType::Local,
        .poseInReferenceSpace = {.orientation = {0.0f, 0.0f, 0.0f, 1.0f} , .position = {0.0f, 0.0f, 0.0f} } });

    //uint32_t size_swapchain = m_ray_swapchain.size();
    auto extent = m_swapchain.vk_view_extent();
    extent.width *= 2;
    m_renderer.create_per_frame_data(context, scene, extent, size_command_buffers);
    m_renderer.create_descriptor_sets(context.descriptor_pool, size_command_buffers);

    for(size_t eye_id = 0u; eye_id < 2u; eye_id++)
    {
        composition_layer_views[eye_id] = xr::CompositionLayerProjectionView{
            .subImage = {
                .swapchain = m_swapchain.swapchain,
                .imageRect = {
                    .extent = m_swapchain.view_extent
                },
                .imageArrayIndex = 0u
            }
        };
    }
    composition_layer_views[1].subImage.imageRect.offset = xr::Offset2Di{
        .x = m_swapchain.view_extent.width,
        .y = 0
    };
    composition_layer = xr::CompositionLayerProjection(
        /*.layerFlags =*/ xr::CompositionLayerFlagBits::None,
        /*.space =*/ m_stage_space,
        /*.viewCount =*/ 2u,
        /*.views =*/ composition_layer_views.data()
    );

    m_input_systems.emplace_back(std::make_unique<Scene_vr_input>(instance.instance, session, m_action_sets));
    m_input_systems.emplace_back(std::make_unique<Ui_vr_input>(instance.instance, session, m_action_sets));
    {
        Suggested_binding suggested_binding(instance.instance);
        for (auto& system : m_input_systems) {
            system->suggest_interaction_profile(instance.instance, suggested_binding);
        }
    }

    session.attachSessionActionSets(xr::SessionActionSetsAttachInfo{ 
        .countActionSets = static_cast<uint32_t>(m_action_sets.size()), 
        .actionSets = m_action_sets.data() });
    for (auto action_set : m_action_sets) {
        m_active_action_sets.push_back(xr::ActiveActionSet{ .actionSet = action_set });
    }
}

Session::~Session()
{
    if (session) {
        session.destroy();
    }
}

void Session::step(xr::Instance instance, Scene& scene, std::vector<std::unique_ptr<System>>& systems)
{
    poll_events(instance);
    draw_frame(scene, systems);
}

void Session::poll_events(xr::Instance instance)
{
    while (true)
    {
        xr::EventDataBuffer data_buffer;
        if (instance.pollEvent(data_buffer) == xr::Result::EventUnavailable) {
            return;
        }

        switch (data_buffer.type)
        {
        case xr::StructureType::EventDataEventsLost: {
            if constexpr (verbose) {
                fmt::print("Event: lost events.\n");
            }
            break;
        }
        case xr::StructureType::EventDataInstanceLossPending: {
            if constexpr (verbose) {
                fmt::print("Event: instance loss pending.\n");
            }
            break;
        }
        case xr::StructureType::EventDataInteractionProfileChanged: {
            if constexpr (verbose) {
                fmt::print("Event: interaction profile changed.\n");
            }
            //reinterpret_cast<xr::EventDataInteractionProfileChanged&>(data_buffer);
            break;
        }
        case xr::StructureType::EventDataReferenceSpaceChangePending: {
            if constexpr (verbose) {
                fmt::print("Event: reference space changed.\n");
            }
            break;
        }
        case xr::StructureType::EventDataSessionStateChanged: {
            handle_state_change(reinterpret_cast<xr::EventDataSessionStateChanged&>(data_buffer));
            break;
        }
        }
    }
}

void Session::draw_frame(Scene& scene, std::vector<std::unique_ptr<System>>& systems)
{
    if (m_session_state == xr::SessionState::Ready || 
        m_session_state == xr::SessionState::Synchronized ||
        m_session_state == xr::SessionState::Visible ||
        m_session_state == xr::SessionState::Focused)
    {
        xr::FrameState frame_state = session.waitFrame({});

        if (m_session_state == xr::SessionState::Focused)
        {
            session.syncActions(xr::ActionsSyncInfo{
               .countActiveActionSets = static_cast<uint32_t>(m_active_action_sets.size()),
               .activeActionSets = m_active_action_sets.data()
            });
            for (auto& system : m_input_systems) {
                system->step(scene, session, frame_state.predictedDisplayTime, m_stage_space, Scene::vr_offset_y);
            }
            std::for_each(systems.begin(), systems.end(), [&scene](auto& system) { system->step(scene); });
        }

        if (frame_state.shouldRender &&
            m_session_state != xr::SessionState::Synchronized)
        {
            session.beginFrame({});
            std::vector<xr::CompositionLayerBaseHeader*> layers_pointers;
            xr::ViewState view_state{};
            auto views = session.locateViewsToVector(
                xr::ViewLocateInfo{
                    .viewConfigurationType = xr::ViewConfigurationType::PrimaryStereo,
                    .displayTime = frame_state.predictedDisplayTime,
                    .space = m_stage_space },
                    &(view_state.operator XrViewState & ())
                    );
            for (size_t eye_id = 0u; eye_id < 2u; eye_id++)
            {
                views[eye_id].pose.position.y += Scene::vr_offset_y;
                scene.scene_global.eyes[eye_id].pose = views[eye_id].pose;
                scene.scene_global.eyes[eye_id].fov = views[eye_id].fov;
                composition_layer_views[eye_id].pose = views[eye_id].pose;
                composition_layer_views[eye_id].fov = views[eye_id].fov;
            }

            size_t command_pool_id = m_command_pools.find_next();
            auto& command_buffer = m_command_pools.command_buffers[command_pool_id];

            uint32_t swapchain_index = m_swapchain.swapchain.acquireSwapchainImage({});

            m_swapchain.swapchain.waitSwapchainImage({ .timeout = xr::Duration::infinite() });

            m_renderer.update_per_frame_data(scene, command_pool_id);

            auto total_extent = m_swapchain.vk_view_extent();
            total_extent.width *= 2;
            m_renderer.start_recording(command_buffer, scene);
            m_renderer.barrier_vr_swapchain(command_buffer, m_swapchain.vk_images[swapchain_index]);
            m_renderer.trace(command_buffer, scene, command_pool_id, total_extent);
            m_mirror.copy(command_buffer, m_renderer.per_frame[command_pool_id].storage_image.image, command_pool_id, m_swapchain.vk_view_extent());
            m_renderer.copy_to_vr_swapchain(command_buffer, m_swapchain.vk_images[swapchain_index], command_pool_id, total_extent);
            m_renderer.end_recording(command_buffer, command_pool_id);
           
            command_buffer.end();

            m_mirror.present(command_buffer, m_command_pools.fences[command_pool_id], command_pool_id);

            m_swapchain.swapchain.releaseSwapchainImage({});
            layers_pointers.push_back(reinterpret_cast<xr::CompositionLayerBaseHeader*>(&composition_layer));
            session.endFrame(xr::FrameEndInfo{
                .displayTime = frame_state.predictedDisplayTime,
                .environmentBlendMode = xr::EnvironmentBlendMode::Opaque,
                .layerCount = static_cast<uint32_t>(layers_pointers.size()),
                .layers = layers_pointers.data() });
        }
        else {
            session.beginFrame({});
            session.endFrame(xr::FrameEndInfo{
                .displayTime = frame_state.predictedDisplayTime,
                .environmentBlendMode = xr::EnvironmentBlendMode::Opaque,
                .layerCount = 0u,
                .layers = nullptr });
        }


    }
}

void Session::handle_state_change(xr::EventDataSessionStateChanged& event_stage_changed)
{
    if constexpr (verbose) {
        fmt::print("Event: state changed to {}\n", xr::to_string_literal(event_stage_changed.state));
    }
    m_session_state = event_stage_changed.state;
    switch (m_session_state)
    {
        case xr::SessionState::Ready: {
            session.beginSession(xr::SessionBeginInfo {
                    .primaryViewConfigurationType = xr::ViewConfigurationType::PrimaryStereo
                });
            break;
        }
        case xr::SessionState::Stopping: {
            session.endSession();
            break;
        }
    default:
        break;
    }
}

}