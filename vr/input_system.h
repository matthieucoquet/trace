#pragma once
#include "vr_common.h"

class Scene;

namespace vr
{

class Input_system
{
public:
    Input_system() = default;
    Input_system(const Input_system& other) = delete;
    Input_system(Input_system&& other) = delete;
    Input_system& operator=(const Input_system& other) = delete;
    Input_system& operator=(Input_system&& other) = delete;
    virtual ~Input_system() = default;

    virtual void step(Scene& scene, xr::Session session, xr::Time display_time) = 0;
};

}