include(GoogleTest)

# ---- Test doubles shared by unit tests
add_library(periph_test_support STATIC
    support/fake_transport.cpp
    support/fake_g502.cpp)
target_include_directories(periph_test_support PUBLIC support)
target_link_libraries(periph_test_support PUBLIC periph::core)
periph_warnings(periph_test_support)

# ---- Unit tests (no hardware)
add_executable(periph-unit-tests
    unit/channel_test.cpp
    unit/device_test.cpp
    unit/endpoint_test.cpp)
target_link_libraries(periph-unit-tests PRIVATE periph_test_support GTest::gtest_main)
periph_warnings(periph-unit-tests)
gtest_discover_tests(periph-unit-tests PROPERTIES LABELS unit)

# ---- Hardware tests: skipped unless PERIPH_HW_TEST=1 (see docs/testing/hardware.md)
add_executable(periph-hwtest
    hw/hw_env.cpp
    hw/hw_probe_test.cpp)
target_include_directories(periph-hwtest PRIVATE hw)
target_link_libraries(periph-hwtest PRIVATE periph::core GTest::gtest_main)
periph_warnings(periph-hwtest)
gtest_discover_tests(periph-hwtest PROPERTIES LABELS hw)

# ---- Architecture: Qt only under ui/
add_test(NAME architecture.no_qt_outside_ui
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/CheckNoQt.cmake)
set_tests_properties(architecture.no_qt_outside_ui PROPERTIES LABELS unit)
