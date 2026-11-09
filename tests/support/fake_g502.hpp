#pragma once
// Simulated G502 LIGHTSPEED speaking HID++ 2.0, built from the feature table
// read from real hardware with periph-probe (wired, PID C08D, HID++ 4.2).
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

    std::vector<FakeFeature> features;

    FakeG502();

    // Feature index of a feature id, or 0 when absent.
    [[nodiscard]] std::uint8_t index_of(std::uint16_t id) const;

    std::vector<Frame> handle(std::span<const std::uint8_t> request) const;

    // A transport wired to this simulator (the simulator must outlive it).
    std::unique_ptr<FakeTransport> transport() const;
};

// Simulated HID++ 1.0 receiver with no reachable device: answers 0x8F
// "invalid sub-id" at 0xFF and the given error for every paired slot.
std::unique_ptr<FakeTransport> fake_receiver_transport(std::uint8_t slot_error);

} // namespace periph::test
