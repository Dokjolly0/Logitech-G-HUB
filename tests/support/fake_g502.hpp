#pragma once
// Simulated G502 LIGHTSPEED speaking HID++ 2.0, built from the feature tables
// read from real hardware with periph-probe (HID++ 4.2): wired (PID C08D) by
// default, wireless (receiver slot 1) with wireless().
// It grows with each implemented feature (onboard memory image, LEDs, ...).

#include "fake_transport.hpp"

#include <memory>
#include <string>

namespace periph::test {

struct FakeFeature {
    std::uint16_t id;
    std::uint8_t type;
    std::uint8_t version;
};

class FakeG502 {
public:
    std::uint8_t device_index = hidpp::kDirectIndex;
    std::string name = "G502 LIGHTSPEED Wireless Gaming Mouse";
    std::uint16_t battery_mv = 4121;
    std::uint8_t battery_flags = 0x80; // external power, charging
    std::uint16_t dpi = 800;
    std::uint16_t default_dpi = 800;
    std::uint8_t report_rate_ms = 1;
    std::uint8_t report_rate_mask = 0x8B; // 1, 2, 4, 8 ms
    // Pings to ignore before answering, like a wireless mouse waking from sleep.
    mutable int sleeping_pings = 0;

    std::vector<FakeFeature> features;

    FakeG502();

    // The table seen through the Lightspeed receiver: no 0x00C2 at index 13,
    // 0x2121 (hi-res wheel) appended, so every feature after 12 moves down.
    static FakeG502 wireless(std::uint8_t slot = 1);

    // Feature index of a feature id, or 0 when absent.
    [[nodiscard]] std::uint8_t index_of(std::uint16_t id) const;

    std::vector<Frame> handle(std::span<const std::uint8_t> request) const;

    // A transport wired to this simulator (the simulator must outlive it).
    std::unique_ptr<FakeTransport> transport() const;

    // Like transport(), but behaves like a Linux hid-logitech-dj child node:
    // the "kernel" overwrites the device index of every request with
    // device_index (the receiver slot), and replies carry that slot.
    std::unique_ptr<FakeTransport> dj_child_transport() const;
};

// Simulated HID++ 1.0 receiver with no reachable device: answers 0x8F
// "invalid sub-id" at 0xFF and the given error for every paired slot.
std::unique_ptr<FakeTransport> fake_receiver_transport(std::uint8_t slot_error);

} // namespace periph::test
