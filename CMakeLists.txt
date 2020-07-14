cmake_minimum_required (VERSION 3.16)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

project (trace)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OpenXR CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# import shaderc as a module
# Should switch to a config when shaderc supports it
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(shaderc MODULE REQUIRED)

add_executable (trace)
target_sources(trace PRIVATE main.cpp

                             vulkan/acceleration_structure.cpp vulkan/acceleration_structure.h
                             vulkan/allocation.cpp vulkan/allocation.h
                             vulkan/command_buffer.h
                             vulkan/context.cpp vulkan/context.h
                             vulkan/desktop_mirror.cpp vulkan/desktop_mirror.h
                             vulkan/desktop_swapchain.cpp vulkan/desktop_swapchain.h
                             vulkan/imgui_render.cpp vulkan/imgui_render.h
                             vulkan/raytracing_pipeline.cpp vulkan/raytracing_pipeline.h
                             vulkan/renderer.cpp vulkan/renderer.h
                             vulkan/vk_common.h

                             vr/input_system.h
                             vr/ui_input_system.cpp vr/ui_input_system.h
                             vr/main_input_system.cpp vr/main_input_system.h
                             vr/instance.cpp vr/instance.h
                             vr/session.cpp vr/session.h
                             vr/vr_swapchain.cpp vr/vr_swapchain.h
                             vr/vr_common.h

                             engine.cpp engine.h
                             entities.cpp entities.h
                             input_keyboard_system.cpp input_keyboard_system.h
                             scene.cpp scene.h
                             shader_types.h
                             shader_system.cpp shader_system.h
                             system.h
                             ui_system.cpp ui_system.h
                             window.cpp window.h
 )

# We don't want to use the target Vulkan::Vulkan and xr::openxr_loader
# because this target would link against the static Vulkan/OpenXR library
# vulkan-hpp/openxr-hpp use explicit dynamic linking so we just need to give the include dir here
# See vk::DynamicLoader in vulkan.hpp for more details
target_include_directories(trace SYSTEM PUBLIC ${Vulkan_INCLUDE_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR}/include)

# DON'T COMMIT, just to avoid copy header each time during dev
target_include_directories(trace SYSTEM PUBLIC "C:/code/external/fork_openxr_hpp/include")

target_link_libraries(trace PRIVATE glm glfw OpenXR::openxr_loader shaderc::shaderc_combined imgui::imgui)
# target_link_libraries(trace PRIVATE glm glfw OpenXR::headers shaderc::shaderc_combined imgui::imgui)
target_compile_features(trace PRIVATE cxx_std_20)

get_filename_component(shader_locations shaders ABSOLUTE)
set_source_files_properties(shader_system.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}")
#set_source_files_properties(vulkan/shader_compile.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}")

if(MSVC)
    set_target_properties(trace PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:trace>)
    target_compile_options(trace PRIVATE /experimental:external /external:anglebrackets /external:W0)
    target_compile_options(trace PRIVATE /W4 /WX /permissive- /w15038)
    target_compile_options(trace PRIVATE /EHsc)
    target_compile_options(trace PRIVATE /Zc:preprocessor)
endif()

### Create empty target for shaders ###
file(GLOB_RECURSE shader_list shaders/*)
add_custom_target(shaders ALL SOURCES ${shader_list})
