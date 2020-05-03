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
# find_package(Stb REQUIRED)

add_executable (trace)
target_sources(trace PRIVATE main.cpp
                                acceleration_structure.cpp acceleration_structure.h
                                context.cpp context.h
                                engine.cpp engine.h
                                raytracing_pipeline.cpp raytracing_pipeline.h
                                renderer.cpp renderer.h
                                scene.cpp scene.h
                                swapchain.cpp swapchain.h
                                vr.cpp vr.h
                                window.cpp window.h
                                common.h
                                allocation.cpp allocation.h
                                command_buffer.h
                                shader_types.h
                                )

# We don't want to use the target Vulkan::Vulkan
# because this target would link against the static Vulkan library
# vulkan-hpp use explicit dynamic linking so we just need to give the include dir here
# See vk::DynamicLoader in vulkan.hpp for more details
target_include_directories(trace SYSTEM PUBLIC ${Vulkan_INCLUDE_DIR}
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(trace PRIVATE glm glfw)
target_compile_features(trace PRIVATE cxx_std_20)

if(MSVC)
    set_target_properties(trace PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:trace>)
    target_compile_options(trace PRIVATE /experimental:external /external:anglebrackets /external:W0)
    target_compile_options(trace PRIVATE /W4 /WX /permissive- /w15038)
    target_compile_options(trace PRIVATE /EHsc)
    target_compile_options(trace PUBLIC /Zc:preprocessor)
endif()

### Shaders ###
# Raygen and miss shaders
get_filename_component(raygen_shader shaders/raygen.rgen ABSOLUTE)
get_filename_component(miss_shader shaders/miss.rmiss ABSOLUTE)
set(general_shaders ${raygen_shader} ${miss_shader})
add_custom_target(general_shader ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:trace>/shaders
        COMMAND glslc.exe ${raygen_shader} -o $<TARGET_FILE_DIR:trace>/shaders/raygen.spv
        COMMAND glslc.exe ${miss_shader} -o $<TARGET_FILE_DIR:trace>/shaders/miss.spv
        SOURCES ${general_shaders}
        COMMENT "Compile miss and raygen shaders"  VERBATIM
)

# Pair of intersection and closest hit shaders
file(GLOB_RECURSE hit_shaders shaders/*.rchit)
get_filename_component(common_shader shaders/common.glsl ABSOLUTE)
foreach(shader ${hit_shaders})
    #get_filename_component(name_group ${file_shader} NAME)
    get_filename_component(name_group ${shader} NAME_WE)
    get_filename_component(chit_shader shaders/${name_group}.rchit ABSOLUTE)
    get_filename_component(int_shader shaders/${name_group}.rint ABSOLUTE)
    get_filename_component(group_common_shader shaders/${name_group}.glsl ABSOLUTE)
    add_custom_target(${name_group} ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:trace>/shaders
            COMMAND glslc.exe ${chit_shader} -o $<TARGET_FILE_DIR:trace>/shaders/${name_group}_chit.spv
            COMMAND glslc.exe ${int_shader} -o $<TARGET_FILE_DIR:trace>/shaders/${name_group}_int.spv
            SOURCES ${chit_shader} ${int_shader} ${group_common_shader} ${common_shader}
            COMMENT "Compile shader ${file_shader}"  VERBATIM
    )
endforeach()
