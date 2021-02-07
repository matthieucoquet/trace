cmake_minimum_required(VERSION 3.16)

add_executable(tournesol)

set(SOURCE
    main.cpp
    tournesol.cpp tournesol.hpp)
target_sources(tournesol
    PRIVATE 
    ${SOURCE}
 )
 source_group("sources" FILES ${SOURCE})


target_link_libraries(tournesol
    PRIVATE
    sdf_editor::engine)
target_compile_features(tournesol PRIVATE cxx_std_20)

get_filename_component(shader_locations shaders ABSOLUTE)
get_filename_component(scene_locations scene ABSOLUTE)
set_source_files_properties(tournesol.cpp PROPERTIES COMPILE_DEFINITIONS "SHADER_SOURCE=\"${shader_locations}\";SCENE_JSON=\"${scene_locations}/scene.json\";TEXTURE=\"${scene_locations}/earth.png\"")

if(MSVC)
    set_target_properties(tournesol PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:tournesol>)
endif()

### Create empty target for shaders ###
file(GLOB_RECURSE shader_list shaders/*)
add_custom_target(tournesol_shaders ALL SOURCES ${shader_list} scene/scene.json)

add_custom_command(TARGET tournesol POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:tournesol>/textures
    COMMAND ${CMAKE_COMMAND} -E copy_directory  ${PROJECT_SOURCE_DIR}/engine/textures $<TARGET_FILE_DIR:tournesol>/textures
    COMMENT Copy textures)