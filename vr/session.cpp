#include "session.h"
#include "context.h"
#include "vulkan/context.h"

namespace vr
{
constexpr bool verbose = true;

Session::Session(Context& vr_context, vulkan::Context& vulkan_context)
{
    auto graphic_binding = XrGraphicsBindingVulkanKHR{
        .instance = vulkan_context.instance,
        .physicalDevice = vulkan_context.physical_device,
        .device = vulkan_context.device,
        .queueFamilyIndex = vulkan_context.queue_family,
        .queueIndex = 1u  // Same as present, is that correct
    };
    session = vr_context.instance.createSession(xr::SessionCreateInfo{
           .next = &graphic_binding,
           .systemId = vr_context.system_id
    });
}

Session::~Session()
{
    session.destroy();

}

}