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

TEST_F(G502Fixture, ProbeRetriesPingWhileWirelessMouseWakesUp) {
    sim.sleeping_pings = 2;
    auto dev = Device::probe(channel, kDirectIndex, std::chrono::milliseconds{20});
    ASSERT_TRUE(dev) << dev.error().describe();
    EXPECT_EQ(dev->features().size(), sim.features.size());
}

TEST_F(G502Fixture, ProbeGivesUpAfterLastPingAttempt) {
    sim.sleeping_pings = 3;
    auto dev = Device::probe(channel, kDirectIndex, std::chrono::milliseconds{20});
    ASSERT_FALSE(dev);
    EXPECT_EQ(dev.error().kind, ErrorKind::Timeout);
}

TEST(Receiver, ErrorsAreNotRetried) {
    auto transport = test::fake_receiver_transport(0x09);
    auto* raw = transport.get();
    Channel channel{std::move(transport)};
    ASSERT_FALSE(Device::probe(channel, 1, std::chrono::milliseconds{50}));
    EXPECT_EQ(raw->sent().size(), 1u);
}

// Linux hid-logitech-dj: requests on the child node get the slot as device
// index, so replies never come back at 0xFF. The endpoint's device_index
// (the slot) must be used instead.
TEST(DjChild, AnswersOnlyAtItsSlotIndex) {
    FakeG502 sim;
    sim.device_index = 1;
    Channel channel{sim.dj_child_transport()};

    auto at_ff = Device::probe(channel, kDirectIndex, std::chrono::milliseconds{20}, 1);
    ASSERT_FALSE(at_ff);
    EXPECT_EQ(at_ff.error().kind, ErrorKind::Timeout);

    auto at_slot = Device::probe(channel, 1, std::chrono::milliseconds{20});
    ASSERT_TRUE(at_slot) << at_slot.error().describe();
    auto name = at_slot->name();
    ASSERT_TRUE(name);
    EXPECT_EQ(*name, sim.name);
}

// Wired and wireless feature tables differ (real hardware, Windows and Linux):
// feature indexes must be looked up per connection, never reused.
TEST(Wireless, FeatureIndexesDifferFromWired) {
    FakeG502 wired;
    auto sim = FakeG502::wireless();
    Channel wired_channel{wired.transport()};
    Channel wireless_channel{sim.transport()};
    auto w = Device::probe(wired_channel, kDirectIndex);
    auto r = Device::probe(wireless_channel, 1);
    ASSERT_TRUE(w && r);

    EXPECT_EQ(w->features().size(), r->features().size());
    EXPECT_EQ(w->index_of(feature::AdjustableDpi), r->index_of(feature::AdjustableDpi)); // 12 on both
    EXPECT_EQ(*w->index_of(0x1802) - 1, *r->index_of(0x1802));                       // shifted by 0x00C2
    EXPECT_TRUE(w->index_of(0x00C2));
    EXPECT_FALSE(r->index_of(0x00C2));
    EXPECT_FALSE(w->index_of(0x2121));
    EXPECT_EQ(r->index_of(0x2121), 29);

    auto dpi = read_dpi(*r);
    ASSERT_TRUE(dpi) << dpi.error().describe();
    EXPECT_EQ(dpi->current, sim.dpi);
}
