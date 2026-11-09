# libperiph_core: device protocol and logic. Must never depend on Qt.
add_library(periph_core STATIC
    src/hidpp/hid.cpp
    src/hidpp/hid_transport.cpp
    src/hidpp/hidpp.cpp
    src/hidpp/device.cpp
    src/hidpp/features.cpp)
add_library(periph::core ALIAS periph_core)
target_include_directories(periph_core PUBLIC include)
target_link_libraries(periph_core PRIVATE hidapi::hidapi)
periph_warnings(periph_core)
