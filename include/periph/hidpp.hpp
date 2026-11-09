#pragma once
// HID++ transport: framing, request/response matching and error decoding.
//
// Frame layout (short 0x10 = 7 bytes, long 0x11 = 20 bytes):
//   [report_id, device_index, feature_index, (function << 4) | sw_id, params...]
// HID++ 2.0 error:  [.., device_index, 0xFF, feature_index, fn_sw, error_code]
// HID++ 1.0 error:  [.., device_index, 0x8F, sub_id,        address, error_code]

#include "periph/hid.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace periph::hidpp {

inline constexpr std::uint16_t kLogitechVendorId = 0x046D;
inline constexpr std::uint16_t kUsagePage = 0xFF00;
inline constexpr std::uint16_t kUsageShort = 0x0001;
inline constexpr std::uint16_t kUsageLong = 0x0002;
inline constexpr std::uint8_t kReportShort = 0x10;
inline constexpr std::uint8_t kReportLong = 0x11;
inline constexpr std::size_t kShortSize = 7;
inline constexpr std::size_t kLongSize = 20;
// Index used for a device attached directly (USB cable) or for the receiver itself.
inline constexpr std::uint8_t kDirectIndex = 0xFF;

enum class ErrorKind { Io, Timeout, Hidpp10, Hidpp20, NotSupported };

struct Error {
    ErrorKind kind{};
    std::uint8_t code{};
    std::string detail;
    [[nodiscard]] std::string describe() const;
};

template <class T>
using Result = std::expected<T, Error>;

using Payload = std::array<std::uint8_t, 16>;

// One physical HID++ interface. On Windows it is split into several top-level
// collections (short / long reports) that must be opened separately.
struct Endpoint {
    std::string key;
    std::uint16_t product_id{};
    std::string product;
    std::vector<hid::DeviceInfo> collections;
};

std::vector<Endpoint> find_endpoints();

class Channel {
public:
    static Result<Channel> open(const Endpoint& endpoint);

    Result<Payload> request(std::uint8_t device_index, std::uint8_t feature_index,
                            std::uint8_t function, std::span<const std::uint8_t> params = {},
                            std::chrono::milliseconds timeout = std::chrono::milliseconds{500});

    static void set_trace(bool enabled);

private:
    struct Handle {
        hid::Device device;
        std::uint16_t usage{};
    };
    std::vector<Handle> handles_;
    std::uint8_t sw_id_ = 0x0B; // distinguishes our replies from other HID++ clients
};

} // namespace periph::hidpp
