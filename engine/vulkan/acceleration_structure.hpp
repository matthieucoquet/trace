#pragma once
#include "vk_common.hpp"
#include "vma_buffer.hpp"

struct Scene; 

namespace vulkan
{

class Context;

class Acceleration_structure
{
public:
    vk::AccelerationStructureKHR acceleration_structure;

    Acceleration_structure(Context& context);
    Acceleration_structure(const Acceleration_structure& other) = delete;
    Acceleration_structure(Acceleration_structure&& other) noexcept;
    Acceleration_structure& operator=(const Acceleration_structure& other) = default;
    Acceleration_structure& operator=(Acceleration_structure&& other) noexcept;
    ~Acceleration_structure();

protected:
    vk::Device m_device;
    VmaAllocator m_allocator;
    VmaAllocation m_allocation{};


    Vma_buffer allocate_scratch_buffer() const;
    void allocate_object_memory();
};

class Blas : public Acceleration_structure
{
public:
    vk::DeviceAddress structure_address;

    Blas(Context& context);
    Blas(const Blas& other) = delete;
    Blas(Blas&& other) = delete;
    Blas& operator=(const Blas& other) = delete;
    Blas& operator=(Blas&& other) = delete;
    ~Blas() = default;
private:
    Vma_buffer m_aabbs_buffer;
};

class Tlas : public Acceleration_structure
{
public:
    Tlas(vk::CommandBuffer command_buffer, Context& context, const Blas& blas, const Scene& scene);
    Tlas(const Tlas& other) = delete;
    Tlas(Tlas&& other) = default;
    Tlas& operator=(const Tlas& other) = delete;
    Tlas& operator=(Tlas&& other) = delete;
    ~Tlas() = default;

    void update(vk::CommandBuffer command_buffer, const Scene& scene, bool first_build);
protected:
    Vma_buffer m_instance_buffer;
    Vma_buffer m_scratch_buffer;
    std::vector<vk::AccelerationStructureInstanceKHR> m_instances{};
};

}
