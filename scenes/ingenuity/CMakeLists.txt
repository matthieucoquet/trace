cmake_minimum_required(VERSION 3.16)

add_executable(ingenuity)

set(SOURCE
    main.cpp
    ingenuity.cpp ingenuity.hpp)
target_sources(ingenuity
    PRIVATE 
    ${SOURCE}
 )
 source_group("sources" FILES ${SOURCE})


target_link_libraries(ingenuity
    PRIVATE
    sdf_editor::engine)
target_compile_features(ingenuity PRIVATE cxx_std_20)

get_filename_component(shader_locations shaders ABSOLUTE)
set_source_files_properties(ingenuity.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}")

if(MSVC)
    set_target_properties(ingenuity PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:ingenuity>)
endif()

### Create empty target for shaders ###
file(GLOB_RECURSE shader_list shaders/*)
add_custom_target(ingenuity_shaders ALL SOURCES ${shader_list})

add_custom_command(TARGET ingenuity POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:ingenuity>/textures
    COMMAND ${CMAKE_COMMAND} -E copy_directory  ${PROJECT_SOURCE_DIR}/engine/textures $<TARGET_FILE_DIR:ingenuity>/textures
    COMMENT Copy textures)