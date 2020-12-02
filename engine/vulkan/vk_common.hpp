#pragma once

#define NOMINMAX

// We want to use dynamic dispatch, see vulkan.hpp readme and https://gpuopen.com/reducing-vulkan-api-call-overhead/
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_NO_SMART_HANDLE  //disable because of bug in vulkan_hpp, its fix in master i think (bug in 141 fixed in 147)
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
//#define VK_ENABLE_BETA_EXTENSIONS
//#define GLFW_INCLUDE_VULKAN
//#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>

// VMA shouldn't load function by itself
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <GLFW/glfw3.h>
