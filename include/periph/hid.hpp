#pragma once
// Thin RAII wrapper over hidapi. Platform differences (Windows top-level
// collections vs. Linux hidraw nodes) are handled one layer up, in hidpp.

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>
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

} // namespace periph::hid
