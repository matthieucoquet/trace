#include "acceleration_structure.h"
#include "context.h"
#include "command_buffer.h"
#include "core/scene.h"
#include "core/shader_types.h"

namespace vulkan
{

Acceleration_structure::Acceleration_structure(Context& context) :
    m_device(context.device),
    m_allocator(context.allocator)
{}

Acceleration_structure::~Acceleration_structure()
{
    vmaFreeMemory(m_allocator, m_allocation);
    m_device.destroyAccelerationStructureKHR(acceleration_structure);
}

Allocated_buffer Acceleration_structure::allocate_scratch_buffer() const
{
    // TODO reuse accross several build ? (vk::MemoryBarrier)
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


Blas::Blas(Context& context, vk::Buffer aabb_buffer, uint32_t size_aabbs) :
    Acceleration_structure(context)
{
    auto create_geometry = vk::AccelerationStructureCreateGeometryTypeInfoKHR()
        .setGeometryType(vk::GeometryTypeKHR::eAabbs)
        .setMaxPrimitiveCount(size_aabbs)
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

    vk::DeviceAddress aabb_buffer_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(aabb_buffer));
    auto acceleration_structure_geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eAabbs)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque)  // if set, don't invoque any hit in hit group
        .setGeometry(vk::AccelerationStructureGeometryDataKHR()
            .setAabbs(vk::AccelerationStructureGeometryAabbsDataKHR()
                .setData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(aabb_buffer_address))
                .setStride(sizeof(Aabb))
            ));
    vk::AccelerationStructureGeometryKHR* pointer_acceleration_structure_geometry = &acceleration_structure_geometry;
    auto build_info_offset = vk::AccelerationStructureBuildOffsetInfoKHR()
        .setPrimitiveCount(size_aabbs)
        .setPrimitiveOffset(0)
        .setFirstVertex(0u)
        .setTransformOffset(0u);

    auto scratch_buffer = allocate_scratch_buffer();
    vk::DeviceAddress scratch_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(scratch_buffer.buffer));

    // TODO building on host if better
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


Tlas::Tlas(Context& context, const Blas& blas, const Scene& scene) :
    Acceleration_structure(context)
{
    auto nb_instances = static_cast<uint32_t>(scene.primitives.size());
    auto create_geometry = vk::AccelerationStructureCreateGeometryTypeInfoKHR()
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setMaxPrimitiveCount(nb_instances)
        .setAllowsTransforms(false);
    acceleration_structure = m_device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMaxGeometryCount(1u)
        .setPGeometryInfos(&create_geometry));
    allocate_object_memory();

    std::vector<vk::AccelerationStructureInstanceKHR> instances{};
    for (uint32_t i = 0u; i < scene.primitives.size(); i++)
    {
        instances.push_back(vk::AccelerationStructureInstanceKHR{
            .transform = {
                .matrix = std::array<std::array<float, 4>, 3>{
                    std::array<float, 4>{ 1.0f, 0.0f, 0.0f, scene.primitives[i].center.x },
                    std::array<float, 4>{ 0.0f, 1.0f, 0.0f, scene.primitives[i].center.y },
                    std::array<float, 4>{ 0.0f, 0.0f, 1.0f, scene.primitives[i].center.z }
            } },
            .instanceCustomIndex = i,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = static_cast<std::underlying_type_t<Object_kind>>(scene.kinds[i]),
            .accelerationStructureReference = blas.structure_address
        });
    }

    m_instance_buffer = Allocated_buffer(
        vk::BufferCreateInfo()
        .setSize(sizeof(vk::AccelerationStructureInstanceKHR) * nb_instances)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eRayTracingKHR),
        instances.data(),
        context.device, context.allocator, context.command_pool, context.graphics_queue);
    vk::DeviceAddress instance_buffer_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(m_instance_buffer.buffer));

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
        .setPrimitiveCount(nb_instances)
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);

    auto scratch_buffer = allocate_scratch_buffer();
    vk::DeviceAddress scratch_address = m_device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(scratch_buffer.buffer));

    One_time_command_buffer command_buffer(m_device, context.command_pool, context.graphics_queue);
    command_buffer.command_buffer.buildAccelerationStructureKHR(
        vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setUpdate(false)
        .setDstAccelerationStructure(acceleration_structure)
        .setGeometryArrayOfPointers(false)
        .setGeometryCount(1u)
        .setPpGeometries(&pointer_acceleration_structure_geometry)
        .setScratchData(scratch_address),
        &build_info_offset);
    command_buffer.submit_and_wait_idle();
}

}