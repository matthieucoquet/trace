#pragma once
#include "core/system.hpp"

namespace sdf_editor
{

class Transform_system final : public System
{
public:
    Transform_system(Scene& scene);
    Transform_system(const Transform_system& other) = delete;
    Transform_system(Transform_system&& other) = delete;
    Transform_system& operator=(const Transform_system& other) = delete;
    Transform_system& operator=(Transform_system&& other) = delete;
    ~Transform_system() override = default;
    void step(Scene& scene) override final;
};

}