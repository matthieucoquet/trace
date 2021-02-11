cmake_minimum_required (VERSION 3.16)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OpenXR CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(marl CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# import shaderc as a module
# Should switch to a config when shaderc supports it
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
if (WIN32)
find_package(shaderc MODULE REQUIRED)
endif()

set(USE_AFTERMATH FALSE CACHE BOOL "")
if (USE_AFTERMATH)
find_package(aftermath MODULE REQUIRED)
endif()


add_library(imgui_editor STATIC)
target_sources(imgui_editor
    PRIVATE
    ${PROJECT_SOURCE_DIR}/external/ImGuiColorTextEdit/TextEditor.cpp
    ${PROJECT_SOURCE_DIR}/external/ImGuiColorTextEdit/TextEditor.h)
target_include_directories(imgui_editor SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/external/ImGuiColorTextEdit)
target_link_libraries(imgui_editor PUBLIC imgui::imgui)


add_library(engine STATIC)

set(SOURCE_CORE
    core/scene.hpp
    core/shader.hpp
    core/system.hpp
    core/transform.hpp)
set(SOURCE_ENGINE
    engine/app.cpp engine/app.hpp
    engine/input_glfw_system.cpp engine/input_glfw_system.hpp
    engine/json_system.cpp engine/json_system.hpp
    engine/shader_system.cpp engine/shader_system.hpp
    engine/transform_system.cpp engine/transform_system.hpp
    engine/ui_system.cpp engine/ui_system.hpp
    engine/window.cpp engine/window.hpp)
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
    vulkan/aftermath_crash_tracker.cpp vulkan/aftermath_crash_tracker.hpp
    vulkan/aftermath_database.cpp vulkan/aftermath_database.hpp
    vulkan/acceleration_structure.cpp vulkan/acceleration_structure.hpp
    vulkan/command_buffer.hpp
    vulkan/context.cpp vulkan/context.hpp
    vulkan/desktop_mirror.cpp vulkan/desktop_mirror.hpp
    vulkan/desktop_swapchain.cpp vulkan/desktop_swapchain.hpp
    vulkan/imgui_render.cpp vulkan/imgui_render.hpp
    vulkan/raytracing_pipeline.cpp vulkan/raytracing_pipeline.hpp
    vulkan/renderer.cpp vulkan/renderer.hpp
    vulkan/texture.cpp vulkan/texture.hpp
    vulkan/vma_buffer.cpp vulkan/vma_buffer.hpp
    vulkan/vma_image.cpp vulkan/vma_image.hpp
    vulkan/vk_common.hpp)

target_sources(engine
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
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(engine SYSTEM PUBLIC ${Vulkan_INCLUDE_DIR}
                                                ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (WIN32)
    target_link_libraries(engine PUBLIC shaderc::shaderc_combined)
else()
    target_link_libraries(engine PUBLIC shaderc_combined)
endif()
target_link_libraries(engine
    PRIVATE
    glfw
    imgui::imgui
    PUBLIC
    glm
    marl::marl
    fmt::fmt
    nlohmann_json::nlohmann_json
    OpenXR::openxr_loader
    imgui_editor)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(engine PRIVATE ${TINYGLTF_INCLUDE_DIRS})
target_include_directories(engine PUBLIC "C:\\code\\external\\OpenXR-Hpp\\include")

if(USE_AFTERMATH)
target_link_libraries(engine PUBLIC aftermath::aftermath)
target_compile_definitions(engine PUBLIC USING_AFTERMATH)
endif()
target_compile_features(engine PRIVATE cxx_std_20)
add_library(sdf_editor::engine ALIAS engine)

get_filename_component(shader_locations shaders ABSOLUTE)
set_source_files_properties(engine/shader_system.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}/engine")
set_source_files_properties(vulkan/imgui_render.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}/ui")

set_source_files_properties(engine/gltf_loader.cpp PROPERTIES COMPILE_DEFINITIONS DATA_SOURCE="${PROJECT_SOURCE_DIR}/data")

if(MSVC)
    target_compile_options(engine PRIVATE /experimental:external /external:anglebrackets /external:W0)
    target_compile_options(engine PRIVATE /W4 /WX /permissive- /w15038)
    target_compile_options(engine PRIVATE /EHsc)
    target_compile_options(engine PRIVATE /Zc:preprocessor)

    set_source_files_properties(${PROJECT_SOURCE_DIR}/external/ImGuiColorTextEdit/TextEditor.cpp PROPERTIES COMPILE_FLAGS /WX-)
endif()


### Create empty target for shaders ###
file(GLOB_RECURSE shader_list shaders/*)
add_custom_target(shaders ALL SOURCES ${shader_list})
