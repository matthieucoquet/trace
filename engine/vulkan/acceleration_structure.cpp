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
    m_structure_buffer(std::move(other.m_structure_buffer)),
    m_scratch_buffer(std::move(other.m_scratch_buffer))
{
    other.acceleration_structure = nullptr;
    other.m_device = nullptr;
    other.m_allocator = nullptr;
}

Acceleration_structure& Acceleration_structure::operator=(Acceleration_structure&& other) noexcept
{
    std::swap(acceleration_structure, other.acceleration_structure);
    std::swap(m_device, other.m_device);
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_structure_buffer, other.m_structure_buffer);
    std::swap(m_scratch_buffer, other.m_scratch_buffer);
    return *this;
}
Acceleration_structure::~Acceleration_structure()
{
    if (m_device) {
        m_device.destroyAccelerationStructureKHR(acceleration_structure);
    }
}

Blas::Blas(Context& context) :
    Acceleration_structure(context)
{}

void Blas::build(vk::CommandBuffer command_buffer, bool /*dynamic*/)
{
    vk::AabbPositionsKHR aabb{ -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };

    m_aabbs_buffer = Vma_buffer(
        m_device, m_allocator,
        vk::BufferCreateInfo{
            .size = sizeof(vk::AabbPositionsKHR) * 1,
            .usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress
        },
        VmaAllocationCreateInfo{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
        });
    m_aabbs_buffer.copy(reinterpret_cast<void*>(&aabb), 1 * sizeof(vk::AabbPositionsKHR));
    m_aabbs_buffer.flush();

    vk::DeviceAddress aabb_buffer_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = m_aabbs_buffer.buffer });
    vk::AccelerationStructureGeometryKHR acceleration_structure_geometry{
        .geometryType = vk::GeometryTypeKHR::eAabbs,
        .geometry = vk::AccelerationStructureGeometryDataKHR(vk::AccelerationStructureGeometryAabbsDataKHR{
            .data = vk::DeviceOrHostAddressConstKHR(aabb_buffer_address),
            .stride = sizeof(vk::AabbPositionsKHR) }),
            //.flags = vk::GeometryFlagBitsKHR::eOpaque
    };

    vk::AccelerationStructureBuildGeometryInfoKHR geom_info{
            .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
            .geometryCount = 1u,
            .pGeometries = &acceleration_structure_geometry
    };

    vk::AccelerationStructureBuildSizesInfoKHR build_size = m_device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice, geom_info, 1u);

    m_structure_buffer = Vma_buffer(
        m_device, m_allocator,
        vk::BufferCreateInfo{
            .size = build_size.accelerationStructureSize,
            .usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .sharingMode = vk::SharingMode::eExclusive },
            VmaAllocationCreateInfo{ .usage = VMA_MEMORY_USAGE_GPU_ONLY });
    Vma_buffer scratch_buffer(
        m_device, m_allocator,
        vk::BufferCreateInfo{
            .size = std::max(build_size.buildScratchSize, build_size.updateScratchSize),
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .sharingMode = vk::SharingMode::eExclusive },
            VmaAllocationCreateInfo{ .usage = VMA_MEMORY_USAGE_GPU_ONLY });
    vk::DeviceAddress scratch_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = scratch_buffer.buffer });

    acceleration_structure = m_device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR{
        .createFlags = {},
        .buffer = m_structure_buffer.buffer,
        .offset = 0u,
        .size = build_size.accelerationStructureSize,
        .type = vk::AccelerationStructureTypeKHR::eBottomLevel });

    geom_info.dstAccelerationStructure = acceleration_structure;
    geom_info.scratchData = scratch_address;

    vk::AccelerationStructureBuildRangeInfoKHR build_range{
            .primitiveCount = 1u,
            .primitiveOffset = 0u,
            .firstVertex = 0u,
            .transformOffset = 0u
    };
    command_buffer.buildAccelerationStructuresKHR(geom_info, &build_range);

    structure_address = m_device.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR{
        .accelerationStructure = acceleration_structure });
}


Tlas::Tlas(vk::CommandBuffer command_buffer, Context& context, const Blas& blas, const Scene& scene) :
    Acceleration_structure(context)
{
    auto nb_instances = static_cast<uint32_t>(scene.objects.size());
    for (uint32_t i = 0u; i < scene.objects.size(); i++)
    {
        glm::mat4 inv = glm::inverse(scene.objects_transform[i]);
        m_instances.push_back(vk::AccelerationStructureInstanceKHR{
            .transform = {
                .matrix = std::array<std::array<float, 4>, 3>{
                    std::array<float, 4>{ inv[0].x, inv[1].x, inv[2].x, inv[3].x },
                    std::array<float, 4>{ inv[0].y, inv[1].y, inv[2].y, inv[3].y },
                    std::array<float, 4>{ inv[0].z, inv[1].z, inv[2].z, inv[3].z }
            } },
            .instanceCustomIndex = i,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 2 * static_cast<uint32_t>(scene.objects[i].group_id), // 2 for primary + shadow
            .accelerationStructureReference = blas.structure_address
            });
    }

    m_instance_buffer = Vma_buffer(
        context.device, context.allocator,
        vk::BufferCreateInfo{
            .size = sizeof(vk::AccelerationStructureInstanceKHR) * nb_instances,
            .usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress
        },
        VmaAllocationCreateInfo{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
        });

    vk::DeviceAddress instance_buffer_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = m_instance_buffer.buffer });
    m_acceleration_structure_geometry = vk::AccelerationStructureGeometryKHR{
        .geometryType = vk::GeometryTypeKHR::eInstances,
        .geometry = vk::AccelerationStructureGeometryDataKHR(vk::AccelerationStructureGeometryInstancesDataKHR{
            .arrayOfPointers = false,
            .data = vk::DeviceOrHostAddressConstKHR(instance_buffer_address) })
    };
    //m_acceleration_structure_geometry.geometry.instances.data.hostAddress = nullptr;

    vk::AccelerationStructureBuildGeometryInfoKHR geom_info{
        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
        .geometryCount = 1u,
        .pGeometries = &m_acceleration_structure_geometry
    };
    vk::AccelerationStructureBuildSizesInfoKHR build_size = m_device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice, geom_info, nb_instances);
    m_structure_buffer = Vma_buffer(
        m_device, m_allocator,
        vk::BufferCreateInfo{
            .size = build_size.accelerationStructureSize,
            .usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .sharingMode = vk::SharingMode::eExclusive },
            VmaAllocationCreateInfo{ .usage = VMA_MEMORY_USAGE_GPU_ONLY });
    m_scratch_buffer = Vma_buffer(
        m_device, m_allocator,
        vk::BufferCreateInfo{
            .size = std::max(build_size.buildScratchSize, build_size.updateScratchSize),
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .sharingMode = vk::SharingMode::eExclusive },
            VmaAllocationCreateInfo{ .usage = VMA_MEMORY_USAGE_GPU_ONLY });

    acceleration_structure = m_device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR{
        .createFlags = {},
        .buffer = m_structure_buffer.buffer,
        .offset = 0u,
        .size = build_size.accelerationStructureSize,
        .type = vk::AccelerationStructureTypeKHR::eTopLevel });

    update(command_buffer, scene, true);
}

void Tlas::update(vk::CommandBuffer command_buffer, const Scene& scene, bool first_build)
{
    // Update instances
    for (uint32_t i = 0u; i < scene.objects.size(); i++)
    {
        glm::mat4 inv = glm::inverse(scene.objects_transform[i]);
        m_instances[i].transform.matrix = std::array<std::array<float, 4>, 3>{
            std::array<float, 4>{ inv[0].x, inv[1].x, inv[2].x, inv[3].x },
                std::array<float, 4>{ inv[0].y, inv[1].y, inv[2].y, inv[3].y },
                std::array<float, 4>{ inv[0].z, inv[1].z, inv[2].z, inv[3].z }
        };
    }
    m_instance_buffer.copy(reinterpret_cast<void*>(m_instances.data()), m_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));
    m_instance_buffer.flush();

    vk::DeviceAddress scratch_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = m_scratch_buffer.buffer });

    vk::AccelerationStructureBuildRangeInfoKHR build_range{
            .primitiveCount = static_cast<uint32_t>(scene.objects.size()),
            .primitiveOffset = 0u,
            .firstVertex = 0u,
            .transformOffset = 0u
    };
    command_buffer.buildAccelerationStructuresKHR(
        vk::AccelerationStructureBuildGeometryInfoKHR{
            .type = vk::AccelerationStructureTypeKHR::eTopLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
            .mode = first_build ? vk::BuildAccelerationStructureModeKHR::eBuild : vk::BuildAccelerationStructureModeKHR::eUpdate,
            .srcAccelerationStructure = first_build ? nullptr : acceleration_structure,
            .dstAccelerationStructure = acceleration_structure,
            .geometryCount = 1,
            .pGeometries = &m_acceleration_structure_geometry,
            .scratchData = scratch_address
        },
        &build_range
        );
}

}