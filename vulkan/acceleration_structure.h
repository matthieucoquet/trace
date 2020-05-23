#pragma once
#include "common.h"
#include "allocation.h"

class Scene; 

namespace vulkan
{

class Context;

class Acceleration_structure
{
public:
    vk::AccelerationStructureKHR acceleration_structure;

    Acceleration_structure(Context& context);
    Acceleration_structure(const Acceleration_structure& other) = delete;
    Acceleration_structure(Acceleration_structure&& other) = delete;
    Acceleration_structure& operator=(const Acceleration_structure& other) = default;
    Acceleration_structure& operator=(Acceleration_structure&& other) = default;
    ~Acceleration_structure();

protected:
    vk::Device m_device;
    VmaAllocator m_allocator;
    VmaAllocation m_allocation{};

    Allocated_buffer allocate_scratch_buffer() const;
    void allocate_object_memory();
};

class Blas : public Acceleration_structure
{
public:
    vk::DeviceAddress structure_address;

    Blas(Context& context, vk::Buffer aabb_buffer, uint32_t size_aabbs);
    Blas(const Blas& other) = delete;
    Blas(Blas&& other) = delete;
    Blas& operator=(const Blas& other) = default;
    Blas& operator=(Blas&& other) = default;
    ~Blas() = default;
};

class Tlas : public Acceleration_structure
{
public:
    Tlas(Context& context, const Blas& blas, const Scene& scene);
    Tlas(const Tlas& other) = delete;
    Tlas(Tlas&& other) = delete;
    Tlas& operator=(const Tlas& other) = default;
    Tlas& operator=(Tlas&& other) = default;
    ~Tlas() = default;
protected:
    Allocated_buffer m_instance_buffer;
};

}