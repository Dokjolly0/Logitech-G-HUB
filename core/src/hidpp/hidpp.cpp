#include "periph/hidpp.hpp"

#include <algorithm>
#include <format>
#include <print>

namespace periph::hidpp {
namespace {

bool g_trace = false;

std::string hex(std::span<const std::uint8_t> bytes) {
    std::string out;
    for (auto b : bytes) out += std::format("{:02X} ", b);
    return out;
}

const char* hidpp20_error_name(std::uint8_t code) {
    switch (code) {
        case 0x01: return "unknown";
        case 0x02: return "invalid argument";
        case 0x03: return "out of range";
        case 0x04: return "hardware error";
        case 0x05: return "internal error";
        case 0x06: return "invalid feature index";
        case 0x07: return "invalid function";
        case 0x08: return "busy";
        case 0x09: return "unsupported";
        default: return "?";
    }
}

const char* hidpp10_error_name(std::uint8_t code) {
    switch (code) {
        case 0x01: return "invalid sub-id (HID++ 1.0 only)";
        case 0x02: return "invalid address";
        case 0x03: return "invalid value";
        case 0x04: return "connection failed";
        case 0x05: return "too many devices";
        case 0x06: return "already exists";
        case 0x07: return "busy";
        case 0x08: return "unknown device";
        case 0x09: return "resource error (device offline?)";
        case 0x0A: return "request unavailable";
        case 0x0B: return "invalid parameter";
        case 0x0C: return "wrong PIN";
        default: return "?";
    }
}

} // namespace

std::string Error::describe() const {
    switch (kind) {
        case ErrorKind::Io: return "I/O error: " + detail;
        case ErrorKind::Timeout: return "timeout";
        case ErrorKind::NotSupported: return "not supported: " + detail;
        case ErrorKind::Hidpp10: return std::format("HID++1.0 error 0x{:02X} ({})", code, hidpp10_error_name(code));
        case ErrorKind::Hidpp20: return std::format("HID++2.0 error 0x{:02X} ({})", code, hidpp20_error_name(code));
    }
    return "?";
}

Channel::Channel(std::unique_ptr<ITransport> transport, std::uint8_t sw_id)
    : transport_(std::move(transport)), sw_id_(static_cast<std::uint8_t>(std::clamp<int>(sw_id, 1, 15))) {}

void Channel::set_trace(bool enabled) { g_trace = enabled; }

Result<Payload> Channel::request(std::uint8_t device_index, std::uint8_t feature_index, std::uint8_t function,
                                 std::span<const std::uint8_t> params, std::chrono::milliseconds timeout) {
    using clock = std::chrono::steady_clock;
    const auto fn_sw = static_cast<std::uint8_t>((function & 0x0F) << 4 | sw_id_);

    // Prefer long reports; HID++ 2.0 devices accept them for every function.
    const bool use_long = transport_->supports_long_reports();
    const std::size_t size = use_long ? kLongSize : kShortSize;
    if (params.size() > size - 4) return std::unexpected(Error{ErrorKind::Io, 0, "parameters too long"});

    std::array<std::uint8_t, kLongSize> frame{};
    frame[0] = use_long ? kReportLong : kReportShort;
    frame[1] = device_index;
    frame[2] = feature_index;
    frame[3] = fn_sw;
    std::ranges::copy(params, frame.begin() + 4);

    // Drop stale input so an old reply is never matched to this request.
    std::array<std::uint8_t, 64> buffer{};
    while (transport_->receive(buffer, std::chrono::milliseconds{0}).value_or(0) > 0) {}

    if (g_trace) std::println("  tx {}", hex({frame.data(), size}));
    if (auto sent = transport_->send({frame.data(), size}); !sent)
        return std::unexpected(Error{ErrorKind::Io, 0, sent.error()});

    const auto deadline = clock::now() + timeout;
    while (clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - clock::now());
        auto n = transport_->receive(buffer, std::max(remaining, std::chrono::milliseconds{1}));
        if (!n) return std::unexpected(Error{ErrorKind::Io, 0, n.error()});
        if (*n < 6) continue;
        if (buffer[0] != kReportShort && buffer[0] != kReportLong) continue;
        if (buffer[1] != device_index) continue;
        if (g_trace) std::println("  rx {}", hex({buffer.data(), *n}));

        if (buffer[2] == 0x8F && buffer[3] == feature_index && buffer[4] == fn_sw)
            return std::unexpected(Error{ErrorKind::Hidpp10, buffer[5], {}});
        if (buffer[2] == 0xFF && buffer[3] == feature_index && buffer[4] == fn_sw)
            return std::unexpected(Error{ErrorKind::Hidpp20, buffer[5], {}});
        if (buffer[2] == feature_index && buffer[3] == fn_sw) {
            Payload payload{};
            const auto len = std::min<std::size_t>(*n, kLongSize) - 4;
            std::copy_n(buffer.begin() + 4, len, payload.begin());
            return payload;
        }
        // Anything else is a notification or another client's traffic: ignore it.
    }
    return std::unexpected(Error{ErrorKind::Timeout, 0, {}});
}

} // namespace periph::hidpp
