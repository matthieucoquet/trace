#pragma once

#include "vr_common.h"

#include <openxr/openxr_dynamic_loader.hpp>

namespace vr
{

class Instance
{
public:
    xr::Instance instance;
    xr::SystemId system_id;

    Instance();
    Instance(const Instance& other) = delete;
    Instance(Instance&& other) = delete;
    Instance& operator=(Instance& other) = delete;
    Instance& operator=(Instance&& other) = delete;
    ~Instance();

    void splitAndAppend(char* new_extensions, std::vector<const char*>& required_extensions) const;
private:
    xr::DynamicLoader m_dynamic_loader;
};

}