#include "hw_env.hpp"

#include "periph/hid_transport.hpp"

#include <cstdlib>
#include <string_view>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace periph::hwtest {

using namespace hidpp;

bool enabled() {
    const char* v = std::getenv("PERIPH_HW_TEST");
    return v != nullptr && std::string_view{v} == "1";
}

bool ghub_running() {
#ifdef _WIN32
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W entry{.dwSize = sizeof(PROCESSENTRY32W)};
    bool found = false;
    for (BOOL ok = Process32FirstW(snap, &entry); ok && !found; ok = Process32NextW(snap, &entry))
        found = std::wstring_view{entry.szExeFile} == L"lghub_agent.exe";
    CloseHandle(snap);
    return found;
#else
    return false; // G HUB does not exist outside Windows
#endif
}

std::vector<Target> find_g502_all() {
    constexpr std::uint16_t kWired = 0xC08D;
    constexpr std::uint16_t kReceiver = 0xC539;
    constexpr std::uint16_t kWirelessChild = 0x407F; // Linux hid-logitech-dj node
    struct Path {
        std::uint16_t pid;
        const char* via;
    };
    constexpr Path kPaths[] = {{kWired, "cable"}, {kReceiver, "receiver"}, {kWirelessChild, "hid-logitech-dj node"}};

    std::vector<Target> found;
    const auto endpoints = find_endpoints();
    for (const auto& path : kPaths) {
        for (const auto& ep : endpoints) {
            if (ep.product_id != path.pid) continue;
            const std::uint8_t index = path.pid == kReceiver ? std::uint8_t{1} : ep.device_index;
            auto transport = open_hid_transport(ep);
            if (!transport) continue;
            auto channel = std::make_unique<Channel>(std::move(*transport));
            auto dev = Device::probe(*channel, index);
            if (!dev) continue;
            found.push_back(Target{std::move(channel), std::move(*dev), path.pid != kWired, path.via});
            break;
        }
    }
    return found;
}

std::optional<Target> find_g502() {
    auto all = find_g502_all();
    if (all.empty()) return std::nullopt;
    return std::move(all.front());
}

} // namespace periph::hwtest
