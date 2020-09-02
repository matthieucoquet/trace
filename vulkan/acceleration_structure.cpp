#include "acceleration_structure.hpp"
#include "context.hpp"
#include "command_buffer.hpp"
#include "core/scene.hpp"
#include "core/shader_types.hpp"

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

Allocated_buffer Acceleration_structure::allocate_scratch_buffer() const
{
    auto memory_requirement = m_device.getAccelerationStructureMemoryRequirementsKHR(vk::AccelerationStructureMemoryRequirementsInfoKHR()
        .setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch)
        .setBuildType(vk::AccelerationStructureBuildTypeKHR::eDevice)
        .setAccelerationStructure(acceleration_structure));

    return Allocated_buffer(
        vk::BufferCreateInfo()
        .setSize(memory_requirement.memoryRequirements.size)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eRayTracingKHR)
        .setSharingMode(vk::SharingMode::eExclusive),
        VMA_MEMORY_USAGE_GPU_ONLY, m_device, m_allocator);
}

void Acceleration_structure::allocate_object_memory()
{
    auto memory_requirement = m_device.getAccelerationStructureMemoryRequirementsKHR(vk::AccelerationStructureMemoryRequirementsInfoKHR()
        .setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject)
        .setBuildType(vk::AccelerationStructureBuildTypeKHR::eDevice)
        .setAccelerationStructure(acceleration_structure));
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

    m_device.bindAccelerationStructureMemoryKHR(vk::BindAccelerationStructureMemoryInfoKHR()
        .setAccelerationStructure(acceleration_structure)
        .setMemory(allocation_info.deviceMemory)
        .setMemoryOffset(allocation_info.offset));
}


Blas::Blas(Context& context) :
    Acceleration_structure(context)
{
    auto create_geometry = vk::AccelerationStructureCreateGeometryTypeInfoKHR()
        .setGeometryType(vk::GeometryTypeKHR::eAabbs)
        .setMaxPrimitiveCount(1u)
        .setIndexType(vk::IndexType::eNoneKHR)
        .setMaxVertexCount(0)
        .setVertexFormat(vk::Format::eUndefined)
        .setAllowsTransforms(false);
    acceleration_structure = m_device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMaxGeometryCount(1u)
        .setPGeometryInfos(&create_geometry));
    allocate_object_memory();


    vk::AabbPositionsKHR aabb{ -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
    m_aabbs_buffer = vulkan::Allocated_buffer(
        vk::BufferCreateInfo{
            .size = sizeof(vk::AabbPositionsKHR),
            .usage = vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eRayTracingKHR },
            &aabb,
            context.device, context.allocator, context.command_pool, context.graphics_queue);
    vk::DeviceAddress aabb_buffer_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(m_aabbs_buffer.buffer));
    auto acceleration_structure_geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eAabbs)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque)  // if set, don't invoque any hit in hit group
        .setGeometry(vk::AccelerationStructureGeometryDataKHR()
            .setAabbs(vk::AccelerationStructureGeometryAabbsDataKHR()
                .setData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(aabb_buffer_address))
                .setStride(sizeof(vk::AabbPositionsKHR))
            ));
    vk::AccelerationStructureGeometryKHR* pointer_acceleration_structure_geometry = &acceleration_structure_geometry;
    auto build_info_offset = vk::AccelerationStructureBuildOffsetInfoKHR()
        .setPrimitiveCount(1u)
        .setPrimitiveOffset(0)
        .setFirstVertex(0u)
        .setTransformOffset(0u);

    auto scratch_buffer = allocate_scratch_buffer();
    vk::DeviceAddress scratch_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(scratch_buffer.buffer));

    One_time_command_buffer command_buffer(m_device, context.command_pool, context.graphics_queue);
    command_buffer.command_buffer.buildAccelerationStructureKHR(
        vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setUpdate(false)
        .setDstAccelerationStructure(acceleration_structure)
        .setGeometryArrayOfPointers(false)
        .setGeometryCount(1u)
        .setPpGeometries(&pointer_acceleration_structure_geometry)
        .setScratchData(scratch_address),
        &build_info_offset);
    command_buffer.submit_and_wait_idle();

    structure_address = m_device.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR()
        .setAccelerationStructure(acceleration_structure));
}


Tlas::Tlas(vk::CommandBuffer command_buffer, Context& context, const Blas& blas, const Scene& scene) :
    Acceleration_structure(context)
{
    auto nb_instances = static_cast<uint32_t>(scene.primitives.size());
    auto create_geometry = vk::AccelerationStructureCreateGeometryTypeInfoKHR()
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setMaxPrimitiveCount(nb_instances)
        .setAllowsTransforms(false);
    acceleration_structure = m_device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
        .setMaxGeometryCount(1u)
        .setPGeometryInfos(&create_geometry));
    allocate_object_memory();

    std::vector<vk::AccelerationStructureInstanceKHR> instances{};
    for (uint32_t i = 0u; i < scene.primitives.size(); i++)
    {
        glm::mat4 inv = glm::inverse(scene.primitives[i].world_to_model);
        m_instances.push_back(vk::AccelerationStructureInstanceKHR{
            .transform = {
                .matrix = std::array<std::array<float, 4>, 3>{
                    std::array<float, 4>{ inv[0].x, inv[1].x, inv[2].x, inv[3].x },
                    std::array<float, 4>{ inv[0].y, inv[1].y, inv[2].y, inv[3].y },
                    std::array<float, 4>{ inv[0].z, inv[1].z, inv[2].z, inv[3].z }
            } },
            .instanceCustomIndex = i,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = static_cast<std::underlying_type_t<Object_kind>>(scene.kinds[i]),
            .accelerationStructureReference = blas.structure_address
        });
    }

    m_instance_buffer = Allocated_buffer(
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
        glm::mat4 inv = glm::inverse(scene.primitives[i].world_to_model);
        m_instances[i].transform.matrix = std::array<std::array<float, 4>, 3>{
                std::array<float, 4>{ inv[0].x, inv[1].x, inv[2].x, inv[3].x },
                std::array<float, 4>{ inv[0].y, inv[1].y, inv[2].y, inv[3].y },
                std::array<float, 4>{ inv[0].z, inv[1].z, inv[2].z, inv[3].z }
        };
    }
    m_instance_buffer.copy(reinterpret_cast<void*>(m_instances.data()), m_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));

    vk::DeviceAddress instance_buffer_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(m_instance_buffer.buffer));
    vk::DeviceAddress scratch_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(m_scratch_buffer.buffer));

    auto acceleration_structure_geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque)
        .setGeometry(vk::AccelerationStructureGeometryDataKHR()
            .setInstances(vk::AccelerationStructureGeometryInstancesDataKHR()
                .setArrayOfPointers(false)
                .setData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(instance_buffer_address))
            ));
    vk::AccelerationStructureGeometryKHR* pointer_acceleration_structure_geometry = &acceleration_structure_geometry;
    auto build_info_offset = vk::AccelerationStructureBuildOffsetInfoKHR()
        .setPrimitiveCount(static_cast<uint32_t>(scene.primitives.size()))
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);

    command_buffer.buildAccelerationStructureKHR(
        vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
        .setUpdate(!first_build)
        .setSrcAccelerationStructure(first_build ? nullptr : acceleration_structure)
        .setDstAccelerationStructure(acceleration_structure)
        .setGeometryArrayOfPointers(false)
        .setGeometryCount(1u)
        .setPpGeometries(&pointer_acceleration_structure_geometry)
        .setScratchData(scratch_address),
        &build_info_offset);
}

}