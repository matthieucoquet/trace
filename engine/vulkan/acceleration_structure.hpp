#pragma once
#include "vk_common.hpp"
#include "vma_buffer.hpp"

namespace sdf_editor
{
struct Scene;
}

namespace sdf_editor::vulkan
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
    Vma_buffer m_structure_buffer{};
    Vma_buffer m_scratch_buffer{};
};

class Blas : public Acceleration_structure
{
public:
    vk::DeviceAddress structure_address;
    std::vector<vk::AabbPositionsKHR> aabbs;

    Blas(Context& context);
    Blas(const Blas& other) = delete;
    Blas(Blas&& other) = delete;
    Blas& operator=(const Blas& other) = delete;
    Blas& operator=(Blas&& other) = delete;
    ~Blas() = default;

    void build(vk::CommandBuffer command_buffer);
private:
    Vma_buffer m_aabbs_buffer;
};

class Tlas : public Acceleration_structure
{
public:
    Tlas(vk::CommandBuffer command_buffer, Context& context, const Blas& blas, Scene& scene);
    Tlas(const Tlas& other) = delete;
    Tlas(Tlas&& other) = default;
    Tlas& operator=(const Tlas& other) = delete;
    Tlas& operator=(Tlas&& other) = delete;
    ~Tlas() = default;

    void update(vk::CommandBuffer command_buffer, const Scene& scene, bool first_build);
protected:
    Vma_buffer m_instance_buffer;
    //std::vector<vk::AccelerationStructureInstanceKHR> m_instances{};
    vk::AccelerationStructureGeometryKHR m_acceleration_structure_geometry;
};

}
