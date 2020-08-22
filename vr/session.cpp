#include "session.h"
#include "instance.h"
#include "vulkan/context.h"
#include "main_input_system.h"
#include "ui_input_system.h"
#include <fmt/core.h>
#include <vector>

namespace vr
{
constexpr bool verbose = true;
constexpr size_t size_command_buffers = 4u;

Session::Session(xr::Session new_session, Instance& instance, vulkan::Context& context, Scene& scene) :
    session(new_session),
    //m_input(instance.instance, session),
    m_main_swapchain(instance, session, context),
    m_ui_swapchain(session, context, xr::Extent2Di{ .width = 1000, .height = 1000 }),
    m_renderer(context, scene),
    m_mirror(context, size_command_buffers),
    m_command_buffers(context.device, context.command_pool, context.graphics_queue, size_command_buffers),
    m_imgui_render(context, m_ui_swapchain.vk_view_extent(), m_ui_swapchain.size(), m_ui_swapchain.image_views)
{
    if constexpr (verbose) {
        auto reference_spaces = session.enumerateReferenceSpaces();
        fmt::print("XR runtime supported reference spaces:\n");
        for (auto reference_space : reference_spaces) {
            fmt::print("\t{}\n", xr::to_string_literal(reference_space));
        }
    }
    m_stage_space = session.createReferenceSpace(xr::ReferenceSpaceCreateInfo{
        .referenceSpaceType = xr::ReferenceSpaceType::Stage });

    uint32_t size_swapchain = m_main_swapchain.size();
    auto extent = m_main_swapchain.vk_view_extent();
    extent.width *= 2;
    m_renderer.create_storage_image(context, extent, m_main_swapchain.required_format, size_swapchain);
    m_renderer.create_uniforms(context, size_swapchain);
    m_renderer.create_descriptor_sets(scene, context.descriptor_pool, size_swapchain);

    for(size_t eye_id = 0u; eye_id < 2u; eye_id++)
    {
        composition_layer_views[eye_id] = xr::CompositionLayerProjectionView{
            //.pose = xr::Posef{},
            //.fov = xr::Fovf{},
            .subImage = /*xr::SwapchainSubImage*/ {
                .swapchain = m_main_swapchain.swapchain,
                .imageRect = {
                    //.offset = {},
                    .extent = m_main_swapchain.view_extent
                },
                .imageArrayIndex = 0u
            }
        };
    }
    composition_layer_views[1].subImage.imageRect.offset = xr::Offset2Di{
        .x = m_main_swapchain.view_extent.width,
        .y = 0
    };

    composition_layer_proj = xr::CompositionLayerProjection{
        .space = m_stage_space,
        .viewCount = 2u,
        .views = composition_layer_views.data()
    };
    composition_layer_ui = xr::CompositionLayerQuad{
        .space = m_stage_space,
        .eyeVisibility = xr::EyeVisibility::Both,
        .subImage = /*xr::SwapchainSubImage*/{
                .swapchain = m_ui_swapchain.swapchain,
                .imageRect = /*xr::Rect2Di*/ {
                    //.offset = {},
                    .extent = m_ui_swapchain.view_extent
                },
                .imageArrayIndex = 0u
            },
        .pose = /*xr::Posef*/{.position = {0.5f, 1.5f, -0.5f } },
        .size = /*xr::Extent2Df*/{ .width = 0.4f, .height = 0.4f }
    };

    m_input_systems.emplace_back(std::make_unique<Main_input_system>(instance.instance, session, m_action_sets));
    m_input_systems.emplace_back(std::make_unique<Ui_input_system>(instance.instance, session, m_action_sets));

    session.attachSessionActionSets(xr::SessionActionSetsAttachInfo{ 
        .countActionSets = static_cast<uint32_t>(m_action_sets.size()), 
        .actionSets = m_action_sets.data() });
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
                fmt::print("Event: lost events.");
            }
        }
        case xr::StructureType::EventDataInstanceLossPending: {
            if constexpr (verbose) {
                fmt::print("Event: instance loss pending.");
            }
            break;
        }
        case xr::StructureType::EventDataInteractionProfileChanged: {
            if constexpr (verbose) {
                fmt::print("Event: interaction profile changed.");
            }
            reinterpret_cast<xr::EventDataInteractionProfileChanged&>(data_buffer);
            break;
        }
        case xr::StructureType::EventDataReferenceSpaceChangePending: {
            if constexpr (verbose) {
                fmt::print("Event: reference space changed.");
            }
            break;
        }
        case xr::StructureType::EventDataSessionStateChanged: {
            if constexpr (verbose) {
                fmt::print("Event: state changed.");
            }
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

        for (auto& system : m_input_systems) {
            system->step(scene, session, frame_state.predictedDisplayTime);
        }
        std::for_each(systems.begin(), systems.end(), [&scene](auto& system) { system->step(scene); });


        session.beginFrame({});
        std::vector<xr::CompositionLayerBaseHeader*> layers_pointers;
        if (frame_state.shouldRender &&
            m_session_state != xr::SessionState::Synchronized)
        {
            xr::ViewState view_state{};
            auto views = session.locateViews(
                xr::ViewLocateInfo{
                    .viewConfigurationType = xr::ViewConfigurationType::PrimaryStereo,
                    .displayTime = frame_state.predictedDisplayTime,
                    .space = m_stage_space },
                    &(view_state.operator XrViewState & ())
                    );
            /*fmt::print("OrientationValid {} - PositionValid {} - OrientationTracked {} - PositionTracked {}\n", 
                (bool)(view_state.viewStateFlags ^ xr::ViewStateFlagBits::OrientationValid),
                (bool)(view_state.viewStateFlags ^ xr::ViewStateFlagBits::PositionValid),
                (bool)(view_state.viewStateFlags ^ xr::ViewStateFlagBits::OrientationTracked),
                (bool)(view_state.viewStateFlags ^ xr::ViewStateFlagBits::PositionTracked));*/
            // TODO check view_state
            for (size_t eye_id = 0u; eye_id < 2u; eye_id++)
            {
                scene.scene_global.eyes[eye_id].pose = views[eye_id].pose;
                scene.scene_global.eyes[eye_id].fov = views[eye_id].fov;
                composition_layer_views[eye_id].pose = views[eye_id].pose;
                composition_layer_views[eye_id].fov = views[eye_id].fov;
            }

            size_t command_buffer_id = m_command_buffers.find_available();
            auto command_buffer = m_command_buffers.command_buffers[command_buffer_id];
            uint32_t swapchain_index = m_main_swapchain.swapchain.acquireSwapchainImage({});
            m_main_swapchain.swapchain.waitSwapchainImage({ .timeout = xr::Duration::infinite() });

            m_renderer.update_uniforms(scene, swapchain_index);

            m_renderer.start_recording(command_buffer, m_main_swapchain.vk_images[swapchain_index], swapchain_index, m_main_swapchain.vk_view_extent());
            m_mirror.copy(command_buffer, m_renderer.storage_images[swapchain_index].image, command_buffer_id, m_main_swapchain.vk_view_extent());
            /*static const char* beforeend = "before end recording";
            command_buffer.setCheckpointNV(&beforeend);*/
            m_renderer.end_recording(command_buffer, m_main_swapchain.vk_images[swapchain_index], swapchain_index);
            /*static const char* beforeui = "before ui";
            command_buffer.setCheckpointNV(&beforeui);*/

            swapchain_index = m_ui_swapchain.swapchain.acquireSwapchainImage({});
            m_ui_swapchain.swapchain.waitSwapchainImage({ .timeout = xr::Duration::infinite() });
            ImDrawData* draw_data = ImGui::GetDrawData();
            m_imgui_render.draw(draw_data, command_buffer, swapchain_index);
            command_buffer.end();

            m_mirror.present(command_buffer, m_command_buffers.fences[command_buffer_id], command_buffer_id);
                        
            m_main_swapchain.swapchain.releaseSwapchainImage({});
            m_ui_swapchain.swapchain.releaseSwapchainImage({});
            layers_pointers.push_back(reinterpret_cast<xr::CompositionLayerBaseHeader*>(&composition_layer_proj));
            layers_pointers.push_back(reinterpret_cast<xr::CompositionLayerBaseHeader*>(&composition_layer_ui));
        }


        session.endFrame(xr::FrameEndInfo{
            .displayTime = frame_state.predictedDisplayTime,
            .environmentBlendMode = xr::EnvironmentBlendMode::Opaque,
            .layerCount = static_cast<uint32_t>(layers_pointers.size()),
            .layers = layers_pointers.data()});
    }
}

void Session::handle_state_change(xr::EventDataSessionStateChanged& event_stage_changed)
{
    fmt::print("{}\n", xr::to_string_literal(event_stage_changed.state));
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