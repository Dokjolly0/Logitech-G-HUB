#pragma once
// Thin RAII wrapper over hidapi. Platform differences (Windows top-level
// collections vs. Linux hidraw nodes) are handled one layer up, in hidpp.

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct hid_device_;

namespace periph::hid {

struct DeviceInfo {
    std::string path;
    std::uint16_t vendor_id{};
    std::uint16_t product_id{};
    std::uint16_t usage_page{};
    std::uint16_t usage{};
    int interface_number{-1};
    std::string product;
    std::string serial;
    // Linux only: set for the per-device hidraw node that hid-logitech-dj creates
    // under a Unifying/Lightspeed receiver. The kernel rewrites the device index
    // of every HID++ request sent to that node with this slot (1..6).
    std::optional<std::uint8_t> receiver_slot;
};

// Every HID top-level collection exposed by the given vendor.
std::vector<DeviceInfo> enumerate(std::uint16_t vendor_id);

class Device {
public:
    static std::expected<Device, std::string> open(const std::string& path);

    Device(Device&&) noexcept = default;
    Device& operator=(Device&&) noexcept = default;
    ~Device() = default;

    std::expected<void, std::string> write(std::span<const std::uint8_t> report);
    // Returns the number of bytes read; 0 means the timeout expired.
    std::expected<std::size_t, std::string> read(std::span<std::uint8_t> buffer,
                                                 std::chrono::milliseconds timeout);

private:
    struct Closer {
        void operator()(hid_device_* handle) const noexcept;
    };
    explicit Device(hid_device_* handle) : handle_(handle) {}
    std::unique_ptr<hid_device_, Closer> handle_;
};

namespace detail {
// Fields of /sys/class/hidraw/hidrawN/device/uevent (Linux).
struct Uevent {
    std::string name; // HID_NAME, e.g. "Logitech G502"
    std::string phys; // HID_PHYS, e.g. "usb-0000:00:14.0-1/input2:1"
};
Uevent parse_uevent(std::string_view text);

// hid-logitech-dj child nodes have a HID_PHYS ending in ":<slot>" after the
// interface ("…/input2:1"); receivers and wired devices do not.
std::optional<std::uint8_t> receiver_slot_from_phys(std::string_view phys);
} // namespace detail

} // namespace periph::hid
