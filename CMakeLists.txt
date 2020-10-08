cmake_minimum_required (VERSION 3.16)

if (WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg triplet")
endif()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

project (sdf_editor)
add_subdirectory(engine)
add_subdirectory(scenes)
