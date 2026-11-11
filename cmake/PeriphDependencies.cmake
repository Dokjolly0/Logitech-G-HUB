# Third-party dependencies, pinned. A system package is preferred when present.
include(FetchContent)

# ---- hidapi
# Order: CMake package (Windows, Fedora, source installs), then pkg-config
# (Debian/Ubuntu libhidapi-dev ships only hidapi-hidraw.pc), then a pinned download.
find_package(hidapi 0.14 CONFIG QUIET)
if(NOT hidapi_FOUND AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(HIDAPI_HIDRAW QUIET IMPORTED_TARGET GLOBAL "hidapi-hidraw>=0.14")
        if(HIDAPI_HIDRAW_FOUND)
            add_library(hidapi::hidapi ALIAS PkgConfig::HIDAPI_HIDRAW)
            set(hidapi_FOUND TRUE)
            message(STATUS "hidapi: system hidapi-hidraw ${HIDAPI_HIDRAW_VERSION} (pkg-config)")
        endif()
    endif()
endif()
if(NOT hidapi_FOUND)
    message(STATUS "hidapi: downloading hidapi-0.14.0 (Linux needs libudev-dev to build it)")
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
    unset(CMAKE_POLICY_VERSION_MINIMUM)
endif()

# ---- GoogleTest (tests only)
if(PERIPH_BUILD_TESTS)
    find_package(GTest 1.14 CONFIG QUIET)
    if(NOT GTest_FOUND)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # match MSVC's default /MD runtime
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.15.2
            GIT_SHALLOW    TRUE)
        FetchContent_MakeAvailable(googletest)
    endif()
endif()
