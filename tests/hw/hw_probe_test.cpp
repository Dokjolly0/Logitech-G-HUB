// Read-only hardware checks against a real G502 LIGHTSPEED.
#include "hw_env.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>

using namespace periph;
using namespace periph::hidpp;

TEST(HwProbe, FindsG502WithExpectedFeatures) {
    PERIPH_HW_REQUIRE();
    auto target = hwtest::find_g502();
    ASSERT_TRUE(target) << "G502 LIGHTSPEED not found (cable or receiver)";
    std::cout << "[ hw       ] G502 via " << target->via << '\n';

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

// The receiver node and the Linux hid-logitech-dj node reach the mouse over the
// same wireless link, so they must report the same feature table. (The cable
// does not: its table has 0x00C2 and lacks 0x2121, so indexes differ.)
TEST(HwProbe, WirelessPathsReportTheSameFeatureTable) {
    PERIPH_HW_REQUIRE();
    auto targets = hwtest::find_g502_all();
    std::erase_if(targets, [](const auto& t) { return !t.wireless; });
    if (targets.size() < 2) GTEST_SKIP() << "needs the receiver plus the Linux hid-logitech-dj node";

    const auto& reference = targets.front();
    for (const auto& t : targets) {
        SCOPED_TRACE("via " + t.via);
        EXPECT_EQ(t.device.protocol(), reference.device.protocol());
        ASSERT_EQ(t.device.features().size(), reference.device.features().size());
        for (std::size_t i = 0; i < t.device.features().size(); ++i) {
            EXPECT_EQ(t.device.features()[i].id, reference.device.features()[i].id) << "index " << i;
            EXPECT_EQ(t.device.features()[i].version, reference.device.features()[i].version) << "index " << i;
        }
    }
}
