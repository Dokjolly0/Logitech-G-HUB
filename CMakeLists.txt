cmake_minimum_required(VERSION 3.25)
project(periph VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- hidapi: prefer a system package (Linux distros), otherwise fetch the pinned release.
find_package(hidapi 0.14 CONFIG QUIET)
if(NOT hidapi_FOUND)
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(HIDAPI_WITH_LIBUSB OFF CACHE BOOL "" FORCE)   # Linux: use the hidraw backend
    set(HIDAPI_BUILD_HIDTEST OFF CACHE BOOL "" FORCE)
    # hidapi 0.14 declares cmake_minimum_required(3.1.3), rejected by CMake 4.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_Declare(hidapi
        GIT_REPOSITORY https://github.com/libusb/hidapi.git
        GIT_TAG        hidapi-0.14.0
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(hidapi)
endif()

function(periph_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ---- Core HID++ library (no Qt dependency)
add_library(periph_hidpp STATIC
    src/hidpp/hid.cpp
    src/hidpp/hidpp.cpp
    src/hidpp/device.cpp
    src/hidpp/features.cpp)
target_include_directories(periph_hidpp PUBLIC include)
target_link_libraries(periph_hidpp PRIVATE hidapi::hidapi)
periph_warnings(periph_hidpp)

# ---- Read-only diagnostic CLI
add_executable(periph-probe tools/probe/main.cpp)
target_link_libraries(periph-probe PRIVATE periph_hidpp)
periph_warnings(periph-probe)
