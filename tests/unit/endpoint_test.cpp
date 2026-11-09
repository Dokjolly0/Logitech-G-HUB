#include "periph/hid_transport.hpp"

#include <gtest/gtest.h>

using namespace periph;
using namespace periph::hidpp;

namespace {

hid::DeviceInfo info(std::string path, std::uint16_t usage_page, std::uint16_t usage, std::uint16_t pid = 0xC08D) {
    return hid::DeviceInfo{.path = std::move(path),
                           .vendor_id = 0x046D,
                           .product_id = pid,
                           .usage_page = usage_page,
                           .usage = usage,
                           .interface_number = 2,
                           .product = "G502",
                           .serial = {}};
}

constexpr const char* kGuid = "{4d1e55b2-f16f-11cf-88cb-001111000030}";

} // namespace

TEST(EndpointKey, WindowsCollectionsOfOneInterfaceShareAKey) {
    const std::string col1 = std::string{R"(\\?\HID#VID_046D&PID_C539&MI_02&Col01#7&385cb526&0&0000#)"} + kGuid;
    const std::string col2 = std::string{R"(\\?\HID#VID_046D&PID_C539&MI_02&Col02#7&385cb526&0&0001#)"} + kGuid;
    EXPECT_EQ(detail::endpoint_key(col1), detail::endpoint_key(col2));
}

TEST(EndpointKey, DifferentInterfacesKeepDifferentKeys) {
    const std::string a = std::string{R"(\\?\HID#VID_046D&PID_C539&MI_02&Col01#7&385cb526&0&0000#)"} + kGuid;
    const std::string b = std::string{R"(\\?\HID#VID_046D&PID_C08D&MI_02&Col01#7&4bb5dff&0&0000#)"} + kGuid;
    EXPECT_NE(detail::endpoint_key(a), detail::endpoint_key(b));
}

TEST(EndpointKey, LinuxPathsAreUnchanged) { EXPECT_EQ(detail::endpoint_key("/dev/hidraw3"), "/dev/hidraw3"); }

TEST(GroupEndpoints, WindowsSplitsShortAndLongIntoTwoCollections) {
    const std::string base = R"(\\?\HID#VID_046D&PID_C08D&MI_02&Col0)";
    auto eps = group_endpoints({
        info(base + "1#7&4bb5dff&0&0000#" + kGuid, kUsagePage, kUsageShort),
        info(base + "2#7&4bb5dff&0&0001#" + kGuid, kUsagePage, kUsageLong),
        info(R"(\\?\HID#VID_046D&PID_C08D&MI_00#7&2b1b967b&0&0000#)" + std::string{kGuid}, 0x0001, 0x0002), // mouse
    });
    ASSERT_EQ(eps.size(), 1u);
    EXPECT_EQ(eps[0].product_id, 0xC08D);
    ASSERT_EQ(eps[0].collections.size(), 2u);
}

TEST(GroupEndpoints, LinuxReportsOnePathPerUsageAndKeepsLong) {
    auto eps = group_endpoints({
        info("/dev/hidraw4", kUsagePage, kUsageShort),
        info("/dev/hidraw4", kUsagePage, kUsageLong),
        info("/dev/hidraw2", 0x0001, 0x0002),
    });
    ASSERT_EQ(eps.size(), 1u);
    ASSERT_EQ(eps[0].collections.size(), 1u);
    EXPECT_EQ(eps[0].collections[0].usage, kUsageLong);
}
