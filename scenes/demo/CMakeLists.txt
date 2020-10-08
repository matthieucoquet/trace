cmake_minimum_required(VERSION 3.16)

add_executable(demo)

set(SOURCE
    main.cpp
    demo.cpp demo.hpp)
target_sources(demo
    PRIVATE 
    ${SOURCE}
 )
 source_group("sources" FILES ${SOURCE})


target_link_libraries(demo
    PRIVATE
    sdf_editor::engine)
# target_link_libraries(demo PRIVATE glm glfw OpenXR::headers shaderc::shaderc_combined imgui::imgui)
target_compile_features(demo PRIVATE cxx_std_20)

get_filename_component(shader_locations shaders ABSOLUTE)
set_source_files_properties(demo.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}")

if(MSVC)
    set_target_properties(demo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:demo>)
endif()

### Create empty target for shaders ###
file(GLOB_RECURSE shader_list shaders/*)
add_custom_target(demo_shaders ALL SOURCES ${shader_list})
