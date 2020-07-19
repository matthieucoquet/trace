#pragma once
#include "vulkan/vk_common.h"

//#define XR_NO_PROTOTYPES
#define OPENXR_HPP_DISPATCH_LOADER_DYNAMIC 1
//#define OPENXR_HPP_NO_SMART_HANDLE
#define OPENXR_HPP_NO_CONSTRUCTOR
#define XR_USE_GRAPHICS_API_VULKAN
#include <windows.h>
#define XR_USE_PLATFORM_WIN32
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>
