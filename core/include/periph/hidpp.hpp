#pragma once
// HID++ transport-independent layer: framing, request/response matching and
// error decoding.
//
// Frame layout (short 0x10 = 7 bytes, long 0x11 = 20 bytes):
//   [report_id, device_index, feature_index, (function << 4) | sw_id, params...]
// HID++ 2.0 error:  [.., device_index, 0xFF, feature_index, fn_sw, error_code]
// HID++ 1.0 error:  [.., device_index, 0x8F, sub_id,        address, error_code]

#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>

namespace periph::hidpp {

inline constexpr std::uint16_t kLogitechVendorId = 0x046D;
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

// A bidirectional HID++ pipe to one physical interface. The real
// implementation is HidTransport (hid_transport.hpp); tests use fakes.
class ITransport {
public:
    virtual ~ITransport() = default;
    // True when long (0x11) reports can be sent.
    [[nodiscard]] virtual bool supports_long_reports() const = 0;
    virtual std::expected<void, std::string> send(std::span<const std::uint8_t> frame) = 0;
    // Returns the number of bytes received; 0 means the timeout expired.
    virtual std::expected<std::size_t, std::string> receive(std::span<std::uint8_t> buffer,
                                                            std::chrono::milliseconds timeout) = 0;
};

class Channel {
public:
    explicit Channel(std::unique_ptr<ITransport> transport, std::uint8_t sw_id = 0x0B);

    Result<Payload> request(std::uint8_t device_index, std::uint8_t feature_index,
                            std::uint8_t function, std::span<const std::uint8_t> params = {},
                            std::chrono::milliseconds timeout = std::chrono::milliseconds{500});

    [[nodiscard]] std::uint8_t sw_id() const { return sw_id_; }

    static void set_trace(bool enabled);

private:
    std::unique_ptr<ITransport> transport_;
    std::uint8_t sw_id_; // distinguishes our replies from other HID++ clients (1..15)
};

} // namespace periph::hidpp
