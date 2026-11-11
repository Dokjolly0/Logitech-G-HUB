#pragma once
// Guards and helpers for hardware tests. They run only with PERIPH_HW_TEST=1
// and a G502 LIGHTSPEED attached (cable or receiver). See docs/testing/hardware.md.

#include "periph/device.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace periph::hwtest {

// True when PERIPH_HW_TEST=1.
bool enabled();

// True when the G HUB agent is running (it would fight our writes).
bool ghub_running();

// An open channel plus the probed G502, kept alive together.
struct Target {
    std::unique_ptr<hidpp::Channel> channel;
    hidpp::Device device;
    bool wireless{};
    std::string via; // "cable", "receiver" or "hid-logitech-dj node" (Linux)
};

// Every path that currently reaches the G502 LIGHTSPEED, in preference order:
// cable (C08D, index 0xFF), receiver C539 slot 1, and on Linux the
// hid-logitech-dj node of the paired mouse (PID 407F, its slot index).
std::vector<Target> find_g502_all();

// The first entry of find_g502_all().
std::optional<Target> find_g502();

} // namespace periph::hwtest

// Skip unless hardware tests are enabled. Tests that write must also call
// PERIPH_HW_REQUIRE_EXCLUSIVE().
#define PERIPH_HW_REQUIRE()                                                         \
    do {                                                                            \
        if (!::periph::hwtest::enabled()) GTEST_SKIP() << "set PERIPH_HW_TEST=1";   \
    } while (0)

#define PERIPH_HW_REQUIRE_EXCLUSIVE()                                                      \
    do {                                                                                   \
        PERIPH_HW_REQUIRE();                                                               \
        if (::periph::hwtest::ghub_running()) FAIL() << "close G HUB (lghub_agent) first"; \
    } while (0)
