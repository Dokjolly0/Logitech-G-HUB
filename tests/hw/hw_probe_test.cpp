// Read-only hardware checks against a real G502 LIGHTSPEED.
#include "hw_env.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using namespace periph;
using namespace periph::hidpp;

TEST(HwProbe, FindsG502WithExpectedFeatures) {
    PERIPH_HW_REQUIRE();
    auto target = hwtest::find_g502();
    ASSERT_TRUE(target) << "G502 LIGHTSPEED not found (cable or receiver)";

    auto& dev = target->device;
    EXPECT_GE(dev.protocol().first, 2);
    for (auto id : {feature::DeviceNameType, feature::BatteryVoltage, feature::AdjustableDpi, feature::ReportRate,
                    feature::ColorLedEffects, feature::OnboardProfiles})
        EXPECT_TRUE(dev.index_of(id)) << std::hex << "missing feature 0x" << id;

    auto name = dev.name();
    ASSERT_TRUE(name);
    EXPECT_NE(name->find("G502"), std::string::npos);
}

TEST(HwProbe, ReadingsAreInPlausibleRanges) {
    PERIPH_HW_REQUIRE();
    auto target = hwtest::find_g502();
    ASSERT_TRUE(target);
    auto& dev = target->device;

    auto battery = read_battery(dev);
    ASSERT_TRUE(battery) << battery.error().describe();
    ASSERT_TRUE(battery->millivolts);
    EXPECT_GT(*battery->millivolts, 3000);
    EXPECT_LT(*battery->millivolts, 4500);

    auto dpi = read_dpi(dev);
    ASSERT_TRUE(dpi) << dpi.error().describe();
    EXPECT_GE(dpi->current, 100);
    EXPECT_LE(dpi->current, 25600);

    auto rate = read_report_rate(dev);
    ASSERT_TRUE(rate) << rate.error().describe();
    EXPECT_FALSE(rate->supported_ms.empty());
    EXPECT_NE(std::ranges::find(rate->supported_ms, rate->current_ms), rate->supported_ms.end());
}
