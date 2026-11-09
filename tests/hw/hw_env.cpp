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

std::optional<Target> find_g502() {
    constexpr std::uint16_t kWired = 0xC08D;
    constexpr std::uint16_t kReceiver = 0xC539;
    for (const auto pid : {kWired, kReceiver}) {
        for (const auto& ep : find_endpoints()) {
            if (ep.product_id != pid) continue;
            auto transport = open_hid_transport(ep);
            if (!transport) continue;
            auto channel = std::make_unique<Channel>(std::move(*transport));
            const std::uint8_t index = pid == kWired ? kDirectIndex : 1;
            auto dev = Device::probe(*channel, index);
            if (!dev) continue;
            return Target{std::move(channel), std::move(*dev), pid == kReceiver};
        }
    }
    return std::nullopt;
}

} // namespace periph::hwtest
