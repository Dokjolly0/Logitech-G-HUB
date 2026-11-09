#include "periph/hidpp.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <map>
#include <print>
#include <regex>

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

// Collections of the same interface share a device path except for the
// "&colNN" part and the trailing instance number (Windows only).
std::string endpoint_key(const std::string& path) {
    std::string key = path;
    std::ranges::transform(key, key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    key = std::regex_replace(key, std::regex{"&col[0-9a-f]{2}"}, "");
    key = std::regex_replace(key, std::regex{"&[0-9a-f]{4}#\\{"}, "#{");
    return key;
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

std::vector<Endpoint> find_endpoints() {
    std::map<std::string, Endpoint> grouped;
    for (auto& info : hid::enumerate(kLogitechVendorId)) {
        if (info.usage_page != kUsagePage) continue;
        if (info.usage != kUsageShort && info.usage != kUsageLong) continue;
        auto key = endpoint_key(info.path);
        auto& ep = grouped[key];
        if (ep.collections.empty()) {
            ep.key = key;
            ep.product_id = info.product_id;
            ep.product = info.product;
        }
        // Linux/macOS report the same path once per usage: open it only once.
        const bool duplicate = std::ranges::any_of(ep.collections, [&](const auto& c) { return c.path == info.path; });
        if (!duplicate) ep.collections.push_back(std::move(info));
        else if (info.usage == kUsageLong) {
            for (auto& c : ep.collections)
                if (c.path == info.path) c.usage = kUsageLong; // remember long reports are available
        }
    }
    std::vector<Endpoint> result;
    for (auto& [_, ep] : grouped) result.push_back(std::move(ep));
    return result;
}

Result<Channel> Channel::open(const Endpoint& endpoint) {
    Channel channel;
    for (const auto& c : endpoint.collections) {
        auto dev = hid::Device::open(c.path);
        if (!dev) return std::unexpected(Error{ErrorKind::Io, 0, dev.error()});
        channel.handles_.push_back(Handle{std::move(*dev), c.usage});
    }
    if (channel.handles_.empty()) return std::unexpected(Error{ErrorKind::Io, 0, "no HID++ collection"});
    return channel;
}

void Channel::set_trace(bool enabled) { g_trace = enabled; }

Result<Payload> Channel::request(std::uint8_t device_index, std::uint8_t feature_index, std::uint8_t function,
                                 std::span<const std::uint8_t> params, std::chrono::milliseconds timeout) {
    using clock = std::chrono::steady_clock;
    const std::uint8_t fn_sw = static_cast<std::uint8_t>((function & 0x0F) << 4 | sw_id_);

    // Prefer long reports; HID++ 2.0 devices accept them for every function.
    auto writer = std::ranges::find_if(handles_, [](const Handle& h) { return h.usage == kUsageLong; });
    const bool use_long = writer != handles_.end();
    if (!use_long) writer = handles_.begin();
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
    for (auto& h : handles_)
        while (h.device.read(buffer, std::chrono::milliseconds{0}).value_or(0) > 0) {}

    if (g_trace) std::println("  tx {}", hex({frame.data(), size}));
    if (auto w = writer->device.write({frame.data(), size}); !w)
        return std::unexpected(Error{ErrorKind::Io, 0, w.error()});

    const auto deadline = clock::now() + timeout;
    while (clock::now() < deadline) {
        for (auto& h : handles_) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - clock::now());
            const auto slice = handles_.size() == 1 ? std::max(remaining, std::chrono::milliseconds{1})
                                                    : std::chrono::milliseconds{5};
            auto n = h.device.read(buffer, slice);
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
    }
    return std::unexpected(Error{ErrorKind::Timeout, 0, {}});
}

} // namespace periph::hidpp
