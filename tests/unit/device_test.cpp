#include "fake_g502.hpp"
#include "periph/device.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace periph;
using namespace periph::hidpp;
using periph::test::FakeG502;

namespace {

struct G502Fixture : ::testing::Test {
    FakeG502 sim;
    Channel channel{sim.transport()};

    Device probe() {
        auto dev = Device::probe(channel, kDirectIndex);
        if (!dev) throw std::runtime_error("probe failed: " + dev.error().describe());
        return std::move(*dev);
    }
};

} // namespace

TEST_F(G502Fixture, ProbeReadsProtocolAndFullFeatureTable) {
    auto dev = probe();
    EXPECT_EQ(dev.protocol(), std::make_pair(4, 2));
    ASSERT_EQ(dev.features().size(), sim.features.size());
    for (std::size_t i = 0; i < sim.features.size(); ++i) {
        EXPECT_EQ(dev.features()[i].id, sim.features[i].id) << "index " << i;
        EXPECT_EQ(dev.features()[i].index, i);
        EXPECT_EQ(dev.features()[i].version, sim.features[i].version);
        EXPECT_EQ(dev.features()[i].type, sim.features[i].type);
    }
    EXPECT_EQ(dev.index_of(feature::AdjustableDpi), sim.index_of(0x2201));
    EXPECT_FALSE(dev.index_of(feature::UnifiedBattery));
}

TEST_F(G502Fixture, ReadsNameLongerThanOneChunk) {
    auto dev = probe();
    auto name = dev.name();
    ASSERT_TRUE(name);
    EXPECT_EQ(*name, sim.name);
}

TEST_F(G502Fixture, ReadsKind) {
    auto dev = probe();
    auto kind = dev.kind();
    ASSERT_TRUE(kind);
    EXPECT_STREQ(device_kind_name(*kind), "mouse");
}

TEST_F(G502Fixture, ReadsBatteryVoltageWhileCharging) {
    auto dev = probe();
    auto b = read_battery(dev);
    ASSERT_TRUE(b);
    EXPECT_EQ(b->source_feature, feature::BatteryVoltage);
    EXPECT_EQ(b->millivolts, 4121);
    EXPECT_FALSE(b->percent);
    EXPECT_TRUE(b->charging);
    EXPECT_FALSE(b->full);
}

TEST_F(G502Fixture, BatteryFullAndDischargingFlags) {
    sim.battery_flags = 0x81; // external power, full
    auto dev = probe();
    auto full = read_battery(dev);
    ASSERT_TRUE(full);
    EXPECT_TRUE(full->full);
    EXPECT_FALSE(full->charging);

    sim.battery_flags = 0x00; // on battery
    auto discharging = read_battery(dev);
    ASSERT_TRUE(discharging);
    EXPECT_FALSE(discharging->charging);
    EXPECT_FALSE(discharging->full);
}

TEST_F(G502Fixture, ReadsDpiAndRangeList) {
    sim.dpi = 1600;
    auto dev = probe();
    auto d = read_dpi(dev);
    ASSERT_TRUE(d);
    EXPECT_EQ(d->current, 1600);
    EXPECT_EQ(d->default_dpi, 800);
    EXPECT_EQ(d->supported, "100-25600 step 50");
}

TEST_F(G502Fixture, ReadsReportRate) {
    sim.report_rate_ms = 2;
    auto dev = probe();
    auto r = read_report_rate(dev);
    ASSERT_TRUE(r);
    EXPECT_EQ(r->current_ms, 2);
    EXPECT_EQ(r->supported_ms, (std::vector<int>{1, 2, 4, 8}));
}

TEST_F(G502Fixture, MissingFeatureIsNotSupported) {
    std::erase_if(sim.features, [](const auto& f) { return f.id == 0x1001; });
    auto dev = probe();
    auto b = read_battery(dev);
    ASSERT_FALSE(b);
    EXPECT_EQ(b.error().kind, ErrorKind::NotSupported);
}

TEST(Receiver, ReportsHidpp10AtDirectIndexAndOfflineSlots) {
    Channel channel{test::fake_receiver_transport(0x09)};
    auto direct = Device::probe(channel, kDirectIndex, std::chrono::milliseconds{50});
    ASSERT_FALSE(direct);
    EXPECT_EQ(direct.error().kind, ErrorKind::Hidpp10);
    EXPECT_EQ(direct.error().code, 0x01);

    auto slot = Device::probe(channel, 1, std::chrono::milliseconds{50});
    ASSERT_FALSE(slot);
    EXPECT_EQ(slot.error().code, 0x09);
}
