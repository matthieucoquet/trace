# Find shaderc lib
# Needed because shaderc doesn't provide a config package
# Need to install shaderc via vcpkg first or LunarG vulkan SDK
# -------
# Import shaderc::shaderc_combined

if (FALSE) # To use vcpkg lib
    find_path(shaderc_INCLUDE_DIR
        NAMES shaderc.hpp
        PATH_SUFFIXES shaderc
    )
    find_library(shaderc_LIBRARY_RELEASE
        shaderc_combined
        PATH ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
        NO_CMAKE_PATH  
    )
    find_library(shaderc_LIBRARY_DEBUG
        shaderc_combinedd
        PATH ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
        NO_CMAKE_PATH  
    )
else() # To use lunarg lib, probably more up to date that vcpkg, but can not choose triplet
    set(vulkan_root $ENV{VULKAN_SDK})
    find_path(shaderc_INCLUDE_DIR
        NAMES shaderc.hpp
        PATHS ${vulkan_root}/Include
        PATH_SUFFIXES shaderc
        NO_CMAKE_PATH
    )
    find_library(shaderc_LIBRARY_RELEASE
        shaderc_combined
        PATH ${vulkan_root}/lib
        NO_CMAKE_PATH  
    )
    find_library(shaderc_LIBRARY_DEBUG
        shaderc_combinedd
        PATH ${vulkan_root}/lib
        NO_CMAKE_PATH  
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(shaderc
    FOUND_VAR shaderc_FOUND
    REQUIRED_VARS
    shaderc_LIBRARY_RELEASE
    shaderc_LIBRARY_DEBUG
    shaderc_INCLUDE_DIR
)

if(shaderc_FOUND)
    if (NOT TARGET shaderc::shaderc_combined)
        add_library(shaderc::shaderc_combined UNKNOWN IMPORTED)
    endif()
    set_property(TARGET shaderc::shaderc_combined APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE
    )
    set_target_properties(shaderc::shaderc_combined PROPERTIES
        IMPORTED_LOCATION_RELEASE "${shaderc_LIBRARY_RELEASE}"
    )
    set_property(TARGET shaderc::shaderc_combined APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG
    )
    set_target_properties(shaderc::shaderc_combined PROPERTIES
        IMPORTED_LOCATION_DEBUG "${shaderc_LIBRARY_DEBUG}"
    )
    set_target_properties(shaderc::shaderc_combined PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${shaderc_INCLUDE_DIR}"
  )
endif()