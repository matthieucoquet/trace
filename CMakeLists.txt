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
find_package(fmt CONFIG REQUIRED)
find_package(marl CONFIG REQUIRED)

# import shaderc as a module
# Should switch to a config when shaderc supports it
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(shaderc MODULE REQUIRED)

add_executable (trace)

set(SOURCE_CORE
    core/shader_data.hpp
    core/scene.cpp core/scene.hpp
    core/device_data.hpp
    core/system.hpp
    core/transform_helpers.hpp
    core/window.cpp core/window.hpp)
set(SOURCE_ENGINE
    engine/main.cpp
    engine/engine.cpp engine/engine.hpp
    engine/input_glfw_system.cpp engine/input_glfw_system.hpp
    engine/shader_system.cpp engine/shader_system.hpp
    engine/ui_system.cpp engine/ui_system.hpp)
set(SOURCE_VR
    vr/glm_helpers.hpp
    vr/vr_input.hpp
    vr/ui_vr_input.cpp vr/ui_vr_input.hpp
    vr/scene_vr_input.cpp vr/scene_vr_input.hpp
    vr/instance.cpp vr/instance.hpp
    vr/session.cpp vr/session.hpp
    vr/vr_swapchain.cpp vr/vr_swapchain.hpp
    vr/vr_common.hpp)
set(SOURCE_VULKAN
    vulkan/acceleration_structure.cpp vulkan/acceleration_structure.hpp
    vulkan/command_buffer.hpp
    vulkan/command_checkpoint.hpp
    vulkan/context.cpp vulkan/context.hpp
    vulkan/desktop_mirror.cpp vulkan/desktop_mirror.hpp
    vulkan/desktop_swapchain.cpp vulkan/desktop_swapchain.hpp
    vulkan/imgui_render.cpp vulkan/imgui_render.hpp
    vulkan/raytracing_pipeline.cpp vulkan/raytracing_pipeline.hpp
    vulkan/renderer.cpp vulkan/renderer.hpp
    vulkan/vma_buffer.cpp vulkan/vma_buffer.hpp
    vulkan/vma_image.cpp vulkan/vma_image.hpp
    vulkan/vk_common.hpp)

target_sources(trace
    PRIVATE 
    ${SOURCE_CORE}
    ${SOURCE_ENGINE}
    ${SOURCE_VR}
    ${SOURCE_VULKAN}
 )
 source_group("core" FILES ${SOURCE_CORE})
 source_group("engine" FILES ${SOURCE_ENGINE})
 source_group("vr" FILES ${SOURCE_VR})
 source_group("vulkan" FILES ${SOURCE_VULKAN})

# We don't want to use the target Vulkan::Vulkan and xr::openxr_loader
# because this target would link against the static Vulkan/OpenXR library
# vulkan-hpp/openxr-hpp use explicit dynamic linking so we just need to give the include dir here
# See vk::DynamicLoader in vulkan.hpp for more details
target_include_directories(trace SYSTEM PUBLIC ${Vulkan_INCLUDE_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR}/include)

# DON'T COMMIT, just to avoid copy header each time during dev
target_include_directories(trace SYSTEM PUBLIC "C:/code/external/fork_openxr_hpp/include")

target_link_libraries(trace
    PRIVATE
    glm
    glfw
    # OpenXR::headers
    OpenXR::openxr_loader
    shaderc::shaderc_combined 
    imgui::imgui
    fmt::fmt
    marl::marl)
# target_link_libraries(trace PRIVATE glm glfw OpenXR::headers shaderc::shaderc_combined imgui::imgui)
target_compile_features(trace PRIVATE cxx_std_20)

get_filename_component(shader_locations shaders ABSOLUTE)
set_source_files_properties(engine/shader_system.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}/scene")
set_source_files_properties(vulkan/imgui_render.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}/ui")

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
