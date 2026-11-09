#include "periph/device.hpp"

#include <algorithm>
#include <format>

namespace periph::hidpp {
namespace {

std::uint16_t be16(const Payload& p, std::size_t offset) {
    return static_cast<std::uint16_t>(p[offset] << 8 | p[offset + 1]);
}

Error not_supported(std::uint16_t feature_id) {
    return Error{ErrorKind::NotSupported, 0, std::format("feature 0x{:04X}", feature_id)};
}

} // namespace

Result<Device> Device::probe(Channel& channel, std::uint8_t device_index, std::chrono::milliseconds timeout) {
    Device dev{channel, device_index};

    // Root.GetProtocolVersion(0, 0, ping): HID++ 1.0 devices answer with error 0x8F.
    const std::array<std::uint8_t, 3> ping{0x00, 0x00, 0x5A};
    auto version = channel.request(device_index, 0x00, 0x1, ping, timeout);
    if (!version) return std::unexpected(version.error());
    dev.major_ = (*version)[0];
    dev.minor_ = (*version)[1];

    // Root.GetFeature(FeatureSet)
    const std::array<std::uint8_t, 2> fs_id{0x00, 0x01};
    auto fs = channel.request(device_index, 0x00, 0x0, fs_id);
    if (!fs) return std::unexpected(fs.error());
    const std::uint8_t fs_index = (*fs)[0];
    if (fs_index == 0) return std::unexpected(not_supported(feature::FeatureSet));

    auto count = channel.request(device_index, fs_index, 0x0);
    if (!count) return std::unexpected(count.error());

    dev.features_.push_back({feature::Root, 0, 0, 0});
    for (std::uint8_t i = 1; i <= (*count)[0]; ++i) {
        const std::array<std::uint8_t, 1> idx{i};
        auto info = channel.request(device_index, fs_index, 0x1, idx);
        if (!info) return std::unexpected(info.error());
        dev.features_.push_back({be16(*info, 0), i, (*info)[2], (*info)[3]});
    }
    return dev;
}

std::optional<std::uint8_t> Device::index_of(std::uint16_t feature_id) const {
    auto it = std::ranges::find(features_, feature_id, &FeatureInfo::id);
    if (it == features_.end()) return std::nullopt;
    return it->index;
}

Result<Payload> Device::call(std::uint16_t feature_id, std::uint8_t function, std::span<const std::uint8_t> params) {
    auto idx = index_of(feature_id);
    if (!idx) return std::unexpected(not_supported(feature_id));
    return channel_->request(index_, *idx, function, params);
}

Result<std::string> Device::name() {
    auto length = call(feature::DeviceNameType, 0x0); // GetDeviceNameCount
    if (!length) return std::unexpected(length.error());
    std::string result;
    while (result.size() < (*length)[0]) {
        const std::array<std::uint8_t, 1> offset{static_cast<std::uint8_t>(result.size())};
        auto chunk = call(feature::DeviceNameType, 0x1, offset); // GetDeviceName
        if (!chunk) return std::unexpected(chunk.error());
        const auto before = result.size();
        for (auto c : *chunk) {
            if (c == 0 || result.size() >= (*length)[0]) break;
            result += static_cast<char>(c);
        }
        if (result.size() == before) break;
    }
    return result;
}

Result<std::uint8_t> Device::kind() {
    auto r = call(feature::DeviceNameType, 0x2); // GetDeviceType
    if (!r) return std::unexpected(r.error());
    return (*r)[0];
}

const char* device_kind_name(std::uint8_t kind) {
    static constexpr const char* names[] = {"keyboard", "remote control", "numpad", "mouse", "touchpad",
                                            "trackball", "presenter", "receiver", "headset", "webcam",
                                            "steering wheel", "joystick", "gamepad", "dock", "speaker",
                                            "microphone"};
    return kind < std::size(names) ? names[kind] : "other";
}

// Status byte shared by 0x1000 and 0x1004 (per public HID++ documentation / Solaar).
static void apply_status(BatteryStatus& s, std::uint8_t status) {
    s.charging = status == 0x01 || status == 0x02 || status == 0x04;
    s.full = status == 0x03;
}

Result<BatteryStatus> read_battery(Device& device) {
    BatteryStatus s;
    if (device.index_of(feature::UnifiedBattery)) {
        auto r = device.call(feature::UnifiedBattery, 0x1); // GetStatus
        if (!r) return std::unexpected(r.error());
        s.source_feature = feature::UnifiedBattery;
        s.percent = (*r)[0];
        apply_status(s, (*r)[2]);
        return s;
    }
    if (device.index_of(feature::BatteryVoltage)) {
        auto r = device.call(feature::BatteryVoltage, 0x0); // GetBatteryInfo
        if (!r) return std::unexpected(r.error());
        s.source_feature = feature::BatteryVoltage;
        s.millivolts = be16(*r, 0);
        const std::uint8_t flags = (*r)[2];
        if (flags & 0x80) {           // external power present
            const auto charge = flags & 0x03;
            s.charging = charge == 0;
            s.full = charge == 1;
        }
        return s;
    }
    if (device.index_of(feature::BatteryLevelStatus)) {
        auto r = device.call(feature::BatteryLevelStatus, 0x0); // GetBatteryLevelStatus
        if (!r) return std::unexpected(r.error());
        s.source_feature = feature::BatteryLevelStatus;
        s.percent = (*r)[0];
        apply_status(s, (*r)[2]);
        return s;
    }
    return std::unexpected(not_supported(feature::UnifiedBattery));
}

Result<DpiInfo> read_dpi(Device& device, std::uint8_t sensor) {
    const std::array<std::uint8_t, 1> arg{sensor};
    auto cur = device.call(feature::AdjustableDpi, 0x2, arg); // GetSensorDpi
    if (!cur) return std::unexpected(cur.error());
    DpiInfo info{.current = be16(*cur, 1), .default_dpi = be16(*cur, 3), .supported = {}};

    auto list = device.call(feature::AdjustableDpi, 0x1, arg); // GetSensorDpiList
    if (!list) return std::unexpected(list.error());
    // Values are big-endian words; a word 0xE000|step means "range from the
    // previous value to the next one in increments of step". 0 terminates.
    std::vector<std::string> parts;
    for (std::size_t off = 1; off + 1 < list->size(); off += 2) {
        const auto v = be16(*list, off);
        if (v == 0) break;
        if ((v >> 13) == 0b111 && off + 3 < list->size() && !parts.empty()) {
            const auto upper = be16(*list, off + 2);
            parts.back() = std::format("{}-{} step {}", parts.back(), upper, v & 0x1FFF);
            off += 2;
        } else {
            parts.push_back(std::to_string(v));
        }
    }
    for (std::size_t i = 0; i < parts.size(); ++i) info.supported += (i ? ", " : "") + parts[i];
    return info;
}

Result<ReportRateInfo> read_report_rate(Device& device) {
    auto list = device.call(feature::ReportRate, 0x0); // GetReportRateList: bit n => (n+1) ms
    if (!list) return std::unexpected(list.error());
    auto cur = device.call(feature::ReportRate, 0x1); // GetReportRate
    if (!cur) return std::unexpected(cur.error());
    ReportRateInfo info{.current_ms = (*cur)[0], .supported_ms = {}};
    for (int bit = 0; bit < 8; ++bit)
        if ((*list)[0] & (1 << bit)) info.supported_ms.push_back(bit + 1);
    return info;
}

} // namespace periph::hidpp
