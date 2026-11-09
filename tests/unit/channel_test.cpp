#include "fake_transport.hpp"

#include <gtest/gtest.h>

using namespace periph;
using namespace periph::hidpp;
using periph::test::FakeTransport;
using periph::test::Frame;

namespace {

constexpr std::uint8_t kSw = 0x0B;
constexpr auto kShortTimeout = std::chrono::milliseconds{20};

std::uint8_t fn_sw(std::uint8_t fn) { return static_cast<std::uint8_t>(fn << 4 | kSw); }

} // namespace

TEST(Channel, BuildsLongFrame) {
    auto transport = std::make_unique<FakeTransport>(
        [](std::span<const std::uint8_t> rq) { return std::vector<Frame>{test::long_reply(rq[1], rq[2], rq[3], {0x42})}; });
    auto* fake = transport.get();
    Channel channel{std::move(transport), kSw};

    const std::array<std::uint8_t, 3> params{0xAA, 0xBB, 0xCC};
    auto r = channel.request(0xFF, 0x05, 0x3, params);
    ASSERT_TRUE(r) << r.error().describe();
    EXPECT_EQ((*r)[0], 0x42);

    ASSERT_EQ(fake->sent().size(), 1u);
    const Frame expected{0x11, 0xFF, 0x05, 0x3B, 0xAA, 0xBB, 0xCC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(fake->sent()[0], expected);
}

TEST(Channel, UsesShortFrameWhenLongUnavailable) {
    auto transport = std::make_unique<FakeTransport>(
        [](std::span<const std::uint8_t> rq) {
            return std::vector<Frame>{Frame{0x10, rq[1], rq[2], rq[3], 0x07, 0x00, 0x00}};
        },
        /*long_reports=*/false);
    auto* fake = transport.get();
    Channel channel{std::move(transport), kSw};

    auto r = channel.request(0x01, 0x00, 0x1);
    ASSERT_TRUE(r);
    EXPECT_EQ((*r)[0], 0x07);
    EXPECT_EQ(fake->sent()[0].size(), kShortSize);
    EXPECT_EQ(fake->sent()[0][0], kReportShort);
}

TEST(Channel, RejectsParamsTooLongForShortReport) {
    Channel channel{std::make_unique<FakeTransport>(nullptr, false), kSw};
    const std::array<std::uint8_t, 4> params{1, 2, 3, 4};
    auto r = channel.request(0x01, 0x00, 0x0, params, kShortTimeout);
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().kind, ErrorKind::Io);
}

TEST(Channel, IgnoresNotificationsAndForeignReplies) {
    auto transport = std::make_unique<FakeTransport>([](std::span<const std::uint8_t> rq) {
        return std::vector<Frame>{
            test::long_reply(rq[1], 0x04, 0x00, {0x99}),                  // notification from another feature
            test::long_reply(rq[1], rq[2], 0x21, {1}),                     // same function, other client's sw_id
            test::long_reply(0x02, rq[2], rq[3], {2}),                     // other device index
            test::long_reply(rq[1], rq[2], rq[3], {0x55}),                 // ours
        };
    });
    Channel channel{std::move(transport), kSw};
    auto r = channel.request(0xFF, 0x0C, 0x2);
    ASSERT_TRUE(r);
    EXPECT_EQ((*r)[0], 0x55);
}

TEST(Channel, DropsStaleInputBeforeSending) {
    auto transport = std::make_unique<FakeTransport>(
        [](std::span<const std::uint8_t> rq) { return std::vector<Frame>{test::long_reply(rq[1], rq[2], rq[3], {0x02})}; });
    // A stale reply that would otherwise match this request.
    transport->inject(test::long_reply(0xFF, 0x01, fn_sw(0x0), {0x01}));
    Channel channel{std::move(transport), kSw};
    auto r = channel.request(0xFF, 0x01, 0x0);
    ASSERT_TRUE(r);
    EXPECT_EQ((*r)[0], 0x02);
}

TEST(Channel, DecodesHidpp20Error) {
    Channel channel{std::make_unique<FakeTransport>([](std::span<const std::uint8_t> rq) {
                        return std::vector<Frame>{test::hidpp20_error(rq[1], rq[2], rq[3], 0x07)};
                    }),
                    kSw};
    auto r = channel.request(0xFF, 0x03, 0x9);
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().kind, ErrorKind::Hidpp20);
    EXPECT_EQ(r.error().code, 0x07);
    EXPECT_NE(r.error().describe().find("invalid function"), std::string::npos);
}

TEST(Channel, DecodesHidpp10Error) {
    Channel channel{std::make_unique<FakeTransport>([](std::span<const std::uint8_t> rq) {
                        return std::vector<Frame>{test::hidpp10_error(rq[1], rq[2], rq[3], 0x09)};
                    }),
                    kSw};
    auto r = channel.request(0x01, 0x00, 0x1);
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().kind, ErrorKind::Hidpp10);
    EXPECT_EQ(r.error().code, 0x09);
}

TEST(Channel, TimesOutWhenDeviceIsSilent) {
    Channel channel{std::make_unique<FakeTransport>(nullptr), kSw};
    auto r = channel.request(0x01, 0x00, 0x1, {}, kShortTimeout);
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().kind, ErrorKind::Timeout);
}

TEST(Channel, ClampsSoftwareId) {
    EXPECT_EQ(Channel(std::make_unique<FakeTransport>(nullptr), 0).sw_id(), 1);
    EXPECT_EQ(Channel(std::make_unique<FakeTransport>(nullptr), 0x20).sw_id(), 15);
}
