#pragma once

#include "common.h"

#include <openxr/openxr_dynamic_loader.hpp>

namespace vr
{

class Context
{
public:
	xr::Instance instance;
	xr::SystemId system_id;

	Context();
	~Context();

	void splitAndAppend(char* new_extensions, std::vector<const char*>& required_extensions) const;
private:
	xr::DynamicLoader m_dynamic_loader;

};

}