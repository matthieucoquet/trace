# Find aftermath lib
# -------
# Import aftermath::aftermath

set(aftermath_root $ENV{AFTERMATH_SDK})
find_path(aftermath_INCLUDE_DIR
    NAMES GFSDK_Aftermath_GpuCrashDump.h
    PATHS ${aftermath_root}/include
    NO_CMAKE_PATH
)
find_library(aftermath_LIBRARY
    "GFSDK_Aftermath_Lib.x64"
    PATH ${aftermath_root}/lib/x64
    NO_CMAKE_PATH  
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(aftermath
    FOUND_VAR aftermath_FOUND
    REQUIRED_VARS
    aftermath_LIBRARY
    aftermath_INCLUDE_DIR
)

if(aftermath_FOUND)
    if (NOT TARGET aftermath::aftermath)
        add_library(aftermath::aftermath UNKNOWN IMPORTED)
    endif()
    set_target_properties(aftermath::aftermath PROPERTIES
        IMPORTED_LOCATION "${aftermath_LIBRARY}"
    )
    set_target_properties(aftermath::aftermath PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${aftermath_INCLUDE_DIR}"
  )
endif()