#include "periph/hid.hpp"

#include <hidapi.h>

#include <charconv>
#include <fstream>
#include <mutex>
#include <sstream>

namespace periph::hid {
namespace {

std::string to_utf8(const wchar_t* text) {
    std::string out;
    if (text == nullptr) return out;
    for (; *text != 0; ++text) {
        auto c = static_cast<char32_t>(*text);
        if (c < 0x80) {
            out += static_cast<char>(c);
        } else if (c < 0x800) {
            out += static_cast<char>(0xC0 | (c >> 6));
            out += static_cast<char>(0x80 | (c & 0x3F));
        } else if (c < 0x10000) {
            out += static_cast<char>(0xE0 | (c >> 12));
            out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (c >> 18));
            out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return out;
}

void ensure_init() {
    static std::once_flag flag;
    std::call_once(flag, [] { hid_init(); });
}

std::string error_of(hid_device* handle) {
    auto message = to_utf8(hid_error(handle));
    return message.empty() ? std::string{"unknown HID error"} : message;
}

// Linux: enrich a hidraw entry with what only sysfs knows (DJ slot, kernel name).
void add_sysfs_info([[maybe_unused]] DeviceInfo& info) {
#ifdef __linux__
    const auto slash = info.path.rfind('/');
    if (slash == std::string::npos || !info.path.starts_with("/dev/hidraw")) return;
    std::ifstream file{"/sys/class/hidraw/" + info.path.substr(slash + 1) + "/device/uevent"};
    if (!file) return;
    std::stringstream text;
    text << file.rdbuf();
    const auto uevent = detail::parse_uevent(text.str());
    info.receiver_slot = detail::receiver_slot_from_phys(uevent.phys);
    // hidapi reports the receiver's USB product string for DJ children.
    if (info.receiver_slot && !uevent.name.empty()) info.product = uevent.name;
#endif
}

} // namespace

namespace detail {

Uevent parse_uevent(std::string_view text) {
    Uevent result;
    while (!text.empty()) {
        const auto eol = text.find('\n');
        const auto line = text.substr(0, eol);
        if (line.starts_with("HID_NAME=")) result.name = std::string{line.substr(9)};
        else if (line.starts_with("HID_PHYS=")) result.phys = std::string{line.substr(9)};
        if (eol == std::string_view::npos) break;
        text.remove_prefix(eol + 1);
    }
    return result;
}

std::optional<std::uint8_t> receiver_slot_from_phys(std::string_view phys) {
    const auto input = phys.rfind("/input");
    if (input == std::string_view::npos) return std::nullopt;
    const auto colon = phys.find(':', input);
    if (colon == std::string_view::npos) return std::nullopt;
    const auto digits = phys.substr(colon + 1);
    unsigned slot = 0;
    auto [end, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), slot);
    if (ec != std::errc{} || end != digits.data() + digits.size() || slot < 1 || slot > 6) return std::nullopt;
    return static_cast<std::uint8_t>(slot);
}

} // namespace detail

void Device::Closer::operator()(hid_device_* handle) const noexcept {
    if (handle != nullptr) hid_close(handle);
}

std::vector<DeviceInfo> enumerate(std::uint16_t vendor_id) {
    ensure_init();
    std::vector<DeviceInfo> result;
    hid_device_info* list = hid_enumerate(vendor_id, 0);
    for (auto* it = list; it != nullptr; it = it->next) {
        result.push_back(DeviceInfo{
            .path = it->path ? it->path : "",
            .vendor_id = it->vendor_id,
            .product_id = it->product_id,
            .usage_page = it->usage_page,
            .usage = it->usage,
            .interface_number = it->interface_number,
            .product = to_utf8(it->product_string),
            .serial = to_utf8(it->serial_number),
            .receiver_slot = std::nullopt,
        });
        add_sysfs_info(result.back());
    }
    hid_free_enumeration(list);
    return result;
}

std::expected<Device, std::string> Device::open(const std::string& path) {
    ensure_init();
    hid_device* handle = hid_open_path(path.c_str());
    if (handle == nullptr) return std::unexpected(error_of(nullptr));
    return Device{handle};
}

std::expected<void, std::string> Device::write(std::span<const std::uint8_t> report) {
    if (hid_write(handle_.get(), report.data(), report.size()) < 0)
        return std::unexpected(error_of(handle_.get()));
    return {};
}

std::expected<std::size_t, std::string> Device::read(std::span<std::uint8_t> buffer,
                                                     std::chrono::milliseconds timeout) {
    const int n = hid_read_timeout(handle_.get(), buffer.data(), buffer.size(),
                                   static_cast<int>(timeout.count()));
    if (n < 0) return std::unexpected(error_of(handle_.get()));
    return static_cast<std::size_t>(n);
}

} // namespace periph::hid
