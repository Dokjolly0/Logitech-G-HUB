#include "periph/device.hpp"

// Feature names as published in Logitech's public HID++ 2.0 documentation and
// used by open-source projects (Solaar, libratbag).
namespace periph::hidpp::feature {

const char* name(std::uint16_t id) {
    switch (id) {
        case 0x0000: return "Root";
        case 0x0001: return "FeatureSet";
        case 0x0002: return "FeatureInfo";
        case 0x0003: return "DeviceFwVersion";
        case 0x0005: return "DeviceNameType";
        case 0x0007: return "DeviceFriendlyName";
        case 0x0008: return "KeepAlive";
        case 0x0020: return "ConfigChange";
        case 0x0021: return "UniqueRandomId";
        case 0x00C2: return "DfuControlSigned";
        case 0x00D0: return "Dfu";
        case 0x1000: return "BatteryLevelStatus";
        case 0x1001: return "BatteryVoltage";
        case 0x1004: return "UnifiedBattery";
        case 0x1010: return "ChargingControl";
        case 0x1300: return "LedControl";
        case 0x1802: return "DeviceReset";
        case 0x1806: return "ConfigurableDeviceProperties";
        case 0x1814: return "ChangeHost";
        case 0x1815: return "HostsInfo";
        case 0x1981: return "Backlight";
        case 0x1982: return "Backlight2";
        case 0x1983: return "Backlight3";
        case 0x1B00: return "ReprogControls";
        case 0x1B04: return "ReprogControlsV4";
        case 0x1BC0: return "ReportHidUsage";
        case 0x1D4B: return "WirelessDeviceStatus";
        case 0x1DF3: return "EquadDjDebugInfo";
        case 0x1E00: return "EnableHiddenFeatures";
        case 0x1EB0: return "TfDebug";
        case 0x1F20: return "AdcMeasurement";
        case 0x2110: return "SmartShift";
        case 0x2111: return "SmartShiftEnhanced";
        case 0x2121: return "HiResWheel";
        case 0x2130: return "RatchetWheel";
        case 0x2150: return "Thumbwheel";
        case 0x2201: return "AdjustableDpi";
        case 0x2202: return "ExtendedAdjustableDpi";
        case 0x2205: return "PointerMotionScaling";
        case 0x2230: return "AngleSnapping";
        case 0x2240: return "SurfaceTuning";
        case 0x2400: return "HybridTracking";
        case 0x40A0: return "FnInversion";
        case 0x40A2: return "NewFnInversion";
        case 0x40A3: return "K375sFnInversion";
        case 0x4220: return "LockKeyState";
        case 0x4521: return "DisableKeys";
        case 0x4522: return "DisableKeysByUsage";
        case 0x4530: return "DualPlatform";
        case 0x4531: return "MultiPlatform";
        case 0x4540: return "KeyboardLayout";
        case 0x6100: return "TouchpadRawXy";
        case 0x8010: return "GKey";
        case 0x8020: return "MKeys";
        case 0x8030: return "MacroRecord";
        case 0x8040: return "BrightnessControl";
        case 0x8060: return "ReportRate";
        case 0x8061: return "ExtendedReportRate";
        case 0x8070: return "ColorLedEffects";
        case 0x8071: return "RgbEffects";
        case 0x8080: return "PerKeyLighting";
        case 0x8081: return "PerKeyLightingV2";
        case 0x8090: return "ModeStatus";
        case 0x8100: return "OnboardProfiles";
        case 0x8110: return "MouseButtonSpy";
        case 0x8111: return "LatencyMonitoring";
        case 0x8120: return "GamingAttachments";
        case 0x8123: return "ForceFeedback";
        case 0x8300: return "Sidetone";
        case 0x8310: return "Equalizer";
        case 0x8320: return "HeadsetOut";
        default: return nullptr;
    }
}

} // namespace periph::hidpp::feature
