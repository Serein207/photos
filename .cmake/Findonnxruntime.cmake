# Find ONNX Runtime
# This module defines:
#   onnxruntime_FOUND - whether onnxruntime was found
#   onnxruntime::onnxruntime - imported target

if(onnxruntime_FOUND)
    return()
endif()

# First try to find via official onnxruntime config
find_package(onnxruntime CONFIG QUIET)
if(TARGET onnxruntime::onnxruntime)
    set(onnxruntime_FOUND TRUE)
    message(STATUS "Found onnxruntime via CONFIG")
    return()
endif()

# Try pkg-config
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_ONNXRUNTIME QUIET onnxruntime)
endif()

# Manual search for header
find_path(onnxruntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATHS
        ${PC_ONNXRUNTIME_INCLUDE_DIRS}
        $ENV{ONNXRUNTIME_ROOT}/include
        /usr/local/include/onnxruntime
        /usr/include/onnxruntime
        /opt/homebrew/include/onnxruntime
        /opt/homebrew/opt/onnxruntime/include/onnxruntime
    NO_DEFAULT_PATH
)

# Manual search for library
find_library(onnxruntime_LIBRARY
    NAMES onnxruntime libonnxruntime
    PATHS
        ${PC_ONNXRUNTIME_LIBRARY_DIRS}
        $ENV{ONNXRUNTIME_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /opt/homebrew/lib
        /opt/homebrew/opt/onnxruntime/lib
    NO_DEFAULT_PATH
)

# On Windows, also search for DLL
if(WIN32)
    find_file(onnxruntime_DLL
        NAMES onnxruntime.dll
        PATHS
            $ENV{ONNXRUNTIME_ROOT}
            $ENV{ONNXRUNTIME_ROOT}/lib
            /usr/local/bin
            /usr/bin
        NO_DEFAULT_PATH
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(onnxruntime
    REQUIRED_VARS onnxruntime_LIBRARY onnxruntime_INCLUDE_DIR
)

if(onnxruntime_FOUND AND NOT TARGET onnxruntime::onnxruntime)
    add_library(onnxruntime::onnxruntime INTERFACE IMPORTED)
    target_include_directories(onnxruntime::onnxruntime INTERFACE ${onnxruntime_INCLUDE_DIR})
    target_link_libraries(onnxruntime::onnxruntime INTERFACE ${onnxruntime_LIBRARY})

    # On Windows with DLL, set IMPORTED_LOCATION for proper DLL copying
    if(WIN32 AND onnxruntime_DLL)
        set_target_properties(onnxruntime::onnxruntime PROPERTIES
            IMPORTED_LOCATION "${onnxruntime_DLL}"
        )
    endif()

    message(STATUS "Found onnxruntime")
    message(STATUS "  Include: ${onnxruntime_INCLUDE_DIR}")
    message(STATUS "  Library: ${onnxruntime_LIBRARY}")
    if(WIN32 AND onnxruntime_DLL)
        message(STATUS "  DLL: ${onnxruntime_DLL}")
    endif()
endif()

mark_as_advanced(onnxruntime_INCLUDE_DIR onnxruntime_LIBRARY onnxruntime_DLL)

