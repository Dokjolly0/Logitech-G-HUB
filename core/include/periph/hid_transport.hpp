#pragma once
// ITransport implementation over hidapi, plus discovery of HID++ interfaces.

#include "periph/hid.hpp"
#include "periph/hidpp.hpp"

#include <string>
#include <vector>

namespace periph::hidpp {

inline constexpr std::uint16_t kUsagePage = 0xFF00;
inline constexpr std::uint16_t kUsageShort = 0x0001;
inline constexpr std::uint16_t kUsageLong = 0x0002;

// One physical HID++ interface. On Windows it is split into several top-level
// collections (short / long reports) that must be opened separately; on Linux
// and macOS all usages share one path.
struct Endpoint {
    std::string key;
    std::uint16_t product_id{};
    std::string product;
    // Index to address the device behind this endpoint: 0xFF for a wired
    // device or a receiver, the paired slot for a Linux hid-logitech-dj node.
    std::uint8_t device_index{kDirectIndex};
    std::vector<hid::DeviceInfo> collections;
    [[nodiscard]] bool is_dj_child() const { return device_index != kDirectIndex; }
};

std::vector<Endpoint> find_endpoints();

// Groups raw HID collections into endpoints (pure function, unit-tested).
std::vector<Endpoint> group_endpoints(std::vector<hid::DeviceInfo> infos);

std::expected<std::unique_ptr<ITransport>, std::string> open_hid_transport(const Endpoint& endpoint);

namespace detail {
// Collections of the same interface share a device path except for the
// "&colNN" part and the trailing instance number (Windows only).
std::string endpoint_key(const std::string& path);
} // namespace detail

} // namespace periph::hidpp
