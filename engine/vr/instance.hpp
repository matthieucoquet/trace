#pragma once

#include "vr_common.hpp"
#ifdef TRACE_USE_DYNAMIC_LOADER
#include <openxr/openxr_dynamic_loader.hpp>
#endif

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

    void split_and_append(char* new_extensions, std::vector<const char*>& required_extensions) const;
    float mirror_recommended_ratio() const;
private:
#ifdef TRACE_USE_DYNAMIC_LOADER
    xr::DynamicLoader m_dynamic_loader;
#endif
};

}