#pragma once
#include "vulkan/vk_common.hpp"

//#define TRACE_USE_DYNAMIC_LOADER

#ifdef TRACE_USE_DYNAMIC_LOADER
#define XR_NO_PROTOTYPES
#endif


#define XR_USE_TIMESPEC
#define OPENXR_HPP_DISPATCH_LOADER_DYNAMIC 1
#define OPENXR_HPP_NO_SMART_HANDLE
#define OPENXR_HPP_NO_CONSTRUCTOR
#define XR_USE_GRAPHICS_API_VULKAN
//#include <windows.h>
//#define XR_USE_PLATFORM_WIN32
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>
