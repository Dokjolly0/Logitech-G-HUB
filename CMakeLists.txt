cmake_minimum_required(VERSION 3.25)
project(periph VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PERIPH_BUILD_TESTS "Build unit and hardware tests" ON)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(PeriphDependencies)

function(periph_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# Layering rule: core, ipc, daemon and cli never depend on Qt.
# Only ui/qt may call find_package(Qt6). Enforced by the architecture test.
add_subdirectory(core)
add_subdirectory(tools/probe)

if(PERIPH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
