# periph-probe: read-only HID++ diagnostic.
add_executable(periph-probe main.cpp)
target_link_libraries(periph-probe PRIVATE periph::core)
periph_warnings(periph-probe)
