#include "session.h"
#include "instance.h"
#include "vulkan/context.h"

#include <iostream>
#include <vector>

namespace vr
{
constexpr bool verbose = true;
constexpr size_t size_command_buffers = 4u;

Session::Session(Instance& instance, vulkan::Context& context, Scene& scene) :
    m_renderer(context, scene),
    m_mirror(context, size_command_buffers),
    m_command_buffers(context.device, context.command_pool, context.graphics_queue, size_command_buffers)
{
    auto graphic_binding = xr::GraphicsBindingVulkanKHR{
        .instance = context.instance,
        .physicalDevice = context.physical_device,
        .device = context.device,
        .queueFamilyIndex = context.queue_family,
        .queueIndex = 0u
    };
    session = instance.instance.createSession(xr::SessionCreateInfo{
        .next = xr::get(graphic_binding),
        .systemId = instance.system_id
    });

    if constexpr (verbose) {
        auto reference_spaces = session.enumerateReferenceSpaces();
        std::cout << "XR runtime supported reference spaces:" << std::endl;
        for (auto reference_space : reference_spaces) {
            std::cout << "\t" << xr::to_string_literal(reference_space) << std::endl;
        }
    }
    m_stage_space = session.createReferenceSpace(xr::ReferenceSpaceCreateInfo{
        .referenceSpaceType = xr::ReferenceSpaceType::Stage });

    m_swapchain = Swapchain(instance, session, context);

    uint32_t size_swapchain = m_swapchain.size();
    auto extent = m_swapchain.vk_image_extent();
    m_renderer.create_storage_image(context, extent, m_swapchain.required_format, size_swapchain);
    m_renderer.create_uniforms(context, size_swapchain);
    m_renderer.create_descriptor_sets(scene, size_swapchain);

    for(size_t eye_id = 0u; eye_id < 2u; eye_id++)
    {
        composition_layer_views[eye_id] = xr::CompositionLayerProjectionView{
            .pose = xr::Posef{},
            .fov = xr::Fovf{},
            .subImage = xr::SwapchainSubImage{
                .swapchain = m_swapchain.swapchain,
                .imageRect = xr::Rect2Di {
                    .offset = {},
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

    composition_layer = xr::CompositionLayerProjection{
        .space = m_stage_space,
        .viewCount = 2u,
        .views = composition_layer_views.data()
    };

}

Session::~Session()
{
    if (session) {
        session.destroy();
    }
}

void Session::step(xr::Instance instance, Scene& scene)
{
    poll_events(instance);
    draw_frame(scene);
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
                std::cout << "Event: lost events." << std::endl;
            }
        }
        case xr::StructureType::EventDataInstanceLossPending: {
            if constexpr (verbose) {
                std::cout << "Event: Instance Loss pending." << std::endl;
            }
            break;
        }
        case xr::StructureType::EventDataInteractionProfileChanged: {
            if constexpr (verbose) {
                std::cout << "Event: interaction profile changed." << std::endl;
            }
            break;
        }
        case xr::StructureType::EventDataReferenceSpaceChangePending: {
            if constexpr (verbose) {
                std::cout << "Event: reference space changed." << std::endl;
            }
            break;
        }
        case xr::StructureType::EventDataSessionStateChanged: {
            if constexpr (verbose) {
                std::cout << "Event: state changed." << std::endl;
            }
            handle_state_change(reinterpret_cast<xr::EventDataSessionStateChanged&>(data_buffer));
            break;
        }
        }
    }
}

void Session::draw_frame(Scene& scene)
{
    if (m_session_state == xr::SessionState::Ready || 
        m_session_state == xr::SessionState::Synchronized ||
        m_session_state == xr::SessionState::Visible ||
        m_session_state == xr::SessionState::Focused)
    {
        xr::FrameState frame_state = session.waitFrame({});
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
            // TODO check view_state
            for (size_t eye_id = 0u; eye_id < 2u; eye_id++)
            {
                scene.scene_global.eyes[eye_id].pose = views[eye_id].pose;
                scene.scene_global.eyes[eye_id].fov = views[eye_id].fov;
                composition_layer_views[eye_id].pose = views[eye_id].pose;
                composition_layer_views[eye_id].fov = views[eye_id].fov;
            }

            uint32_t swapchain_index = m_swapchain.swapchain.acquireSwapchainImage({});
            m_swapchain.swapchain.waitSwapchainImage({ .timeout = xr::Duration::infinite() });

            size_t command_buffer_id = m_command_buffers.find_available();
            auto command_buffer = m_command_buffers.command_buffers[command_buffer_id];
            m_renderer.update_uniforms(scene, swapchain_index);
            m_renderer.start_recording(command_buffer, m_swapchain.vk_images[swapchain_index], swapchain_index, m_swapchain.vk_image_extent());
            m_mirror.copy(command_buffer, m_renderer.storage_images[swapchain_index].image, command_buffer_id, m_swapchain.vk_image_extent());
            m_renderer.end_recording(command_buffer, m_swapchain.vk_images[swapchain_index], swapchain_index);
            m_mirror.present(command_buffer, m_command_buffers.fences[command_buffer_id], command_buffer_id);
            m_swapchain.swapchain.releaseSwapchainImage({});
            layers_pointers.push_back(reinterpret_cast<xr::CompositionLayerBaseHeader*>(&composition_layer));
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
    std::cout << xr::to_string_literal(event_stage_changed.state) << std::endl;
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