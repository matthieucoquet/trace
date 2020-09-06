#include "acceleration_structure.hpp"
#include "context.hpp"
#include "command_buffer.hpp"
#include "core/scene.hpp"
#include "core/device_data.hpp"

#include <glm/gtx/string_cast.hpp>
#include <fmt/core.h>

namespace vulkan
{

Acceleration_structure::Acceleration_structure(Context& context) :
    m_device(context.device),
    m_allocator(context.allocator)
{}

Acceleration_structure::Acceleration_structure(Acceleration_structure&& other) noexcept :
    acceleration_structure(other.acceleration_structure),
    m_device(other.m_device),
    m_allocator(other.m_allocator),
    m_allocation(other.m_allocation)
{
    other.acceleration_structure = nullptr;
    other.m_device = nullptr;
    other.m_allocator = nullptr;
    other.m_allocation = nullptr;
}

Acceleration_structure& Acceleration_structure::operator=(Acceleration_structure&& other) noexcept
{
    std::swap(acceleration_structure, other.acceleration_structure);
    std::swap(m_device, other.m_device);
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_allocation, other.m_allocation);
    return *this;
}
Acceleration_structure::~Acceleration_structure()
{
    if (m_device) {
        vmaFreeMemory(m_allocator, m_allocation);
        m_device.destroyAccelerationStructureKHR(acceleration_structure);
    }
}

Vma_buffer Acceleration_structure::allocate_scratch_buffer() const
{
    auto memory_requirement = m_device.getAccelerationStructureMemoryRequirementsKHR(vk::AccelerationStructureMemoryRequirementsInfoKHR{
        .type = vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch,
        .buildType = vk::AccelerationStructureBuildTypeKHR::eDevice,
        .accelerationStructure = acceleration_structure });

    return Vma_buffer(
        vk::BufferCreateInfo{
            .size = memory_requirement.memoryRequirements.size,
            .usage = vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eRayTracingKHR,
            .sharingMode = vk::SharingMode::eExclusive },
        VMA_MEMORY_USAGE_GPU_ONLY, m_device, m_allocator);
}

void Acceleration_structure::allocate_object_memory()
{
    auto memory_requirement = m_device.getAccelerationStructureMemoryRequirementsKHR(vk::AccelerationStructureMemoryRequirementsInfoKHR{
        .type = vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject,
        .buildType = vk::AccelerationStructureBuildTypeKHR::eDevice,
        .accelerationStructure = acceleration_structure });
    ;
    VmaAllocationCreateInfo alloc_create_info{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };
    vmaAllocateMemory(m_allocator,
        reinterpret_cast<const VkMemoryRequirements*>(&memory_requirement.memoryRequirements),
        &alloc_create_info, &m_allocation, nullptr);

    VmaAllocationInfo allocation_info;
    vmaGetAllocationInfo(m_allocator, m_allocation, &allocation_info);
    assert(allocation_info.offset % memory_requirement.memoryRequirements.alignment == 0);

    m_device.bindAccelerationStructureMemoryKHR(vk::BindAccelerationStructureMemoryInfoKHR{
        .accelerationStructure = acceleration_structure,
        .memory = allocation_info.deviceMemory,
        .memoryOffset = allocation_info.offset });
}


Blas::Blas(Context& context) :
    Acceleration_structure(context)
{
    vk::AccelerationStructureCreateGeometryTypeInfoKHR create_geometry {
        .geometryType = vk::GeometryTypeKHR::eAabbs,
        .maxPrimitiveCount = 1u,
        .indexType = vk::IndexType::eNoneKHR,
        .maxVertexCount = 0,
        .vertexFormat = vk::Format::eUndefined,
        .allowsTransforms = false };
    acceleration_structure = m_device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR{
        .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        .maxGeometryCount = 1u,
        .pGeometryInfos = &create_geometry });
    allocate_object_memory();


    vk::AabbPositionsKHR aabb{ -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
    m_aabbs_buffer = vulkan::Vma_buffer(
        vk::BufferCreateInfo{
            .size = sizeof(vk::AabbPositionsKHR),
            .usage = vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eRayTracingKHR },
        &aabb,
        context.device, context.allocator, context.command_pool, context.graphics_queue);
    vk::DeviceAddress aabb_buffer_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = m_aabbs_buffer.buffer });
    vk::AccelerationStructureGeometryKHR acceleration_structure_geometry {
        .geometryType = vk::GeometryTypeKHR::eAabbs,
        .geometry = vk::AccelerationStructureGeometryDataKHR(vk::AccelerationStructureGeometryAabbsDataKHR{
            .data = vk::DeviceOrHostAddressConstKHR(aabb_buffer_address),
            .stride = sizeof(vk::AabbPositionsKHR) }),
        .flags = vk::GeometryFlagBitsKHR::eOpaque  // if set, don't invoque any hit in hit group   
    };
    vk::AccelerationStructureGeometryKHR* pointer_acceleration_structure_geometry = &acceleration_structure_geometry;
    auto build_info_offset = vk::AccelerationStructureBuildOffsetInfoKHR {
        .primitiveCount = 1u,
        .primitiveOffset = 0,
        .firstVertex = 0u,
        .transformOffset = 0u };

    auto scratch_buffer = allocate_scratch_buffer();
    vk::DeviceAddress scratch_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = scratch_buffer.buffer });

    One_time_command_buffer command_buffer(m_device, context.command_pool, context.graphics_queue);
    command_buffer.command_buffer.buildAccelerationStructureKHR(
        vk::AccelerationStructureBuildGeometryInfoKHR{
            .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            .update = false,
            .dstAccelerationStructure = acceleration_structure,
            .geometryArrayOfPointers = false,
            .geometryCount = 1u,
            .ppGeometries = &pointer_acceleration_structure_geometry,
            .scratchData = scratch_address },
        &build_info_offset);
    command_buffer.submit_and_wait_idle();

    structure_address = m_device.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR{
        .accelerationStructure = acceleration_structure });
}


Tlas::Tlas(vk::CommandBuffer command_buffer, Context& context, const Blas& blas, const Scene& scene) :
    Acceleration_structure(context)
{
    auto nb_instances = static_cast<uint32_t>(scene.primitives.size());
    vk::AccelerationStructureCreateGeometryTypeInfoKHR create_geometry{
        .geometryType = vk::GeometryTypeKHR::eInstances,
        .maxPrimitiveCount = nb_instances,
        .allowsTransforms = false };
    acceleration_structure = m_device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR{
        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
        .maxGeometryCount = 1u,
        .pGeometryInfos = &create_geometry });
    allocate_object_memory();

    std::vector<vk::AccelerationStructureInstanceKHR> instances{};
    for (uint32_t i = 0u; i < scene.primitives.size(); i++)
    {
        glm::mat4 inv = glm::inverse(scene.primitive_transform[i]);
        m_instances.push_back(vk::AccelerationStructureInstanceKHR{
            .transform = {
                .matrix = std::array<std::array<float, 4>, 3>{
                    std::array<float, 4>{ inv[0].x, inv[1].x, inv[2].x, inv[3].x },
                    std::array<float, 4>{ inv[0].y, inv[1].y, inv[2].y, inv[3].y },
                    std::array<float, 4>{ inv[0].z, inv[1].z, inv[2].z, inv[3].z }
            } },
            .instanceCustomIndex = i,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = static_cast<uint32_t>(scene.primitive_group_ids[i]),
            .accelerationStructureReference = blas.structure_address
        });
    }

    m_instance_buffer = Vma_buffer(
        vk::BufferCreateInfo{
            .size = sizeof(vk::AccelerationStructureInstanceKHR) * nb_instances,
            .usage = vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eRayTracingKHR
        },
        VMA_MEMORY_USAGE_CPU_TO_GPU, context.device, context.allocator);

    m_scratch_buffer = allocate_scratch_buffer();

    update(command_buffer, scene, true);
}

void Tlas::update(vk::CommandBuffer command_buffer, const Scene& scene, bool first_build)
{
    // Update instances
    for (uint32_t i = 0u; i < scene.primitives.size(); i++)
    {
        glm::mat4 inv = glm::inverse(scene.primitive_transform[i]);
        m_instances[i].transform.matrix = std::array<std::array<float, 4>, 3>{
            std::array<float, 4>{ inv[0].x, inv[1].x, inv[2].x, inv[3].x },
                std::array<float, 4>{ inv[0].y, inv[1].y, inv[2].y, inv[3].y },
                std::array<float, 4>{ inv[0].z, inv[1].z, inv[2].z, inv[3].z }
        };
    }
    m_instance_buffer.copy(reinterpret_cast<void*>(m_instances.data()), m_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));

    vk::DeviceAddress instance_buffer_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = m_instance_buffer.buffer });
    vk::DeviceAddress scratch_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = m_scratch_buffer.buffer });

    vk::AccelerationStructureGeometryKHR acceleration_structure_geometry{
        .geometryType = vk::GeometryTypeKHR::eInstances,
        .geometry = vk::AccelerationStructureGeometryDataKHR(vk::AccelerationStructureGeometryInstancesDataKHR{
            .arrayOfPointers = false,
            .data = vk::DeviceOrHostAddressConstKHR(instance_buffer_address) }),
        .flags = vk::GeometryFlagBitsKHR::eOpaque
    };
    vk::AccelerationStructureGeometryKHR* pointer_acceleration_structure_geometry = &acceleration_structure_geometry;
    vk::AccelerationStructureBuildOffsetInfoKHR build_info_offset{
        .primitiveCount = static_cast<uint32_t>(scene.primitives.size()),
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0 };

    command_buffer.buildAccelerationStructureKHR(
        vk::AccelerationStructureBuildGeometryInfoKHR{
            .type = vk::AccelerationStructureTypeKHR::eTopLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
            .update = !first_build,
            .srcAccelerationStructure = first_build ? nullptr : acceleration_structure,
            .dstAccelerationStructure = acceleration_structure,
            .geometryArrayOfPointers = false,
            .geometryCount = 1u,
            .ppGeometries = &pointer_acceleration_structure_geometry,
            .scratchData = scratch_address },
        &build_info_offset);
}

}