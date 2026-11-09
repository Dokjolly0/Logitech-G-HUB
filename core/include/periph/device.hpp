#pragma once
// A HID++ 2.0 device reachable through a Channel at a given device index,
// plus read helpers for the first features we support.

#include "periph/hidpp.hpp"

#include <optional>
#include <string>
#include <vector>

namespace periph::hidpp {

namespace feature {
inline constexpr std::uint16_t Root = 0x0000;
inline constexpr std::uint16_t FeatureSet = 0x0001;
inline constexpr std::uint16_t DeviceFwVersion = 0x0003;
inline constexpr std::uint16_t DeviceNameType = 0x0005;
inline constexpr std::uint16_t BatteryLevelStatus = 0x1000;
inline constexpr std::uint16_t BatteryVoltage = 0x1001;
inline constexpr std::uint16_t UnifiedBattery = 0x1004;
inline constexpr std::uint16_t AdjustableDpi = 0x2201;
inline constexpr std::uint16_t ReportRate = 0x8060;
inline constexpr std::uint16_t ExtendedReportRate = 0x8061;
inline constexpr std::uint16_t ColorLedEffects = 0x8070;
inline constexpr std::uint16_t RgbEffects = 0x8071;
inline constexpr std::uint16_t OnboardProfiles = 0x8100;

// Human-readable name from public HID++ documentation, or nullptr.
const char* name(std::uint16_t id);
} // namespace feature

struct FeatureInfo {
    std::uint16_t id{};
    std::uint8_t index{};
    std::uint8_t type{};    // bit 7 obsolete, bit 6 hidden, bit 5 engineering
    std::uint8_t version{};
};

class Device {
public:
    // Pings the device and reads its full feature table.
    static Result<Device> probe(Channel& channel, std::uint8_t device_index,
                                std::chrono::milliseconds timeout = std::chrono::milliseconds{400});

    [[nodiscard]] std::uint8_t index() const { return index_; }
    [[nodiscard]] std::pair<int, int> protocol() const { return {major_, minor_}; }
    [[nodiscard]] const std::vector<FeatureInfo>& features() const { return features_; }
    [[nodiscard]] std::optional<std::uint8_t> index_of(std::uint16_t feature_id) const;

    Result<Payload> call(std::uint16_t feature_id, std::uint8_t function,
                         std::span<const std::uint8_t> params = {});

    Result<std::string> name();
    Result<std::uint8_t> kind();

private:
    Device(Channel& channel, std::uint8_t index) : channel_(&channel), index_(index) {}
    Channel* channel_;
    std::uint8_t index_;
    int major_{}, minor_{};
    std::vector<FeatureInfo> features_;
};

const char* device_kind_name(std::uint8_t kind);

struct BatteryStatus {
    std::optional<int> percent;
    std::optional<int> millivolts;
    bool charging{};
    bool full{};
    std::uint16_t source_feature{};
};
Result<BatteryStatus> read_battery(Device& device);

struct DpiInfo {
    int current{};
    int default_dpi{};
    std::string supported; // e.g. "100-25600 step 50" or "400, 800, 1600"
};
Result<DpiInfo> read_dpi(Device& device, std::uint8_t sensor = 0);

struct ReportRateInfo {
    int current_ms{};
    std::vector<int> supported_ms;
};
Result<ReportRateInfo> read_report_rate(Device& device);

} // namespace periph::hidpp
