// periph-probe: read-only HID++ diagnostic. Lists every Logitech HID++
// interface, the devices behind it, their features and a few basic readings.
// It never sends a "set" command.

#include "periph/device.hpp"
#include "periph/hid_transport.hpp"

#include <algorithm>
#include <format>
#include <print>
#include <string_view>

using namespace periph::hidpp;

namespace {

void report_device(Device& dev) {
    const auto [major, minor] = dev.protocol();
    auto name = dev.name();
    auto kind = dev.kind();
    std::println("  [index 0x{:02X}] {} ({}) - HID++ {}.{}", dev.index(), name ? *name : "?",
                 kind ? device_kind_name(*kind) : "?", major, minor);

    std::println("    features ({}):", dev.features().size());
    for (const auto& f : dev.features()) {
        const char* n = feature::name(f.id);
        std::println("      #{:<3} 0x{:04X} v{} {}{}{}", f.index, f.id, f.version, n ? n : "(unknown)",
                     (f.type & 0x40) ? " [hidden]" : "", (f.type & 0x20) ? " [engineering]" : "");
    }

    if (auto b = read_battery(dev)) {
        std::string value = b->percent ? std::format("{}%", *b->percent) : std::format("{} mV", *b->millivolts);
        std::println("    battery: {}{}{} (via 0x{:04X})", value, b->charging ? ", charging" : "",
                     b->full ? ", full" : "", b->source_feature);
    } else if (b.error().kind != ErrorKind::NotSupported) {
        std::println("    battery: {}", b.error().describe());
    }

    if (auto d = read_dpi(dev)) {
        std::println("    dpi: {} (default {}), supported: {}", d->current, d->default_dpi, d->supported);
    } else if (d.error().kind != ErrorKind::NotSupported) {
        std::println("    dpi: {}", d.error().describe());
    }

    if (auto r = read_report_rate(dev)) {
        std::string list;
        for (auto ms : r->supported_ms) list += std::format("{}{} Hz", list.empty() ? "" : ", ", 1000 / ms);
        std::println("    report rate: {} Hz, supported: {}", 1000 / std::max(r->current_ms, 1), list);
    } else if (r.error().kind != ErrorKind::NotSupported) {
        std::println("    report rate: {}", r.error().describe());
    }
}

} // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (std::string_view{argv[i]} == "--trace") Channel::set_trace(true);

    auto endpoints = find_endpoints();
    if (endpoints.empty()) {
        std::println("No Logitech HID++ interface found. On Linux, check the udev rule (see README).");
        return 1;
    }

    for (const auto& ep : endpoints) {
        std::println("\n== {} (PID {:04X}, {} collection(s))", ep.product, ep.product_id, ep.collections.size());
        auto transport = open_hid_transport(ep);
        if (!transport) {
            std::println("  cannot open: {}", transport.error());
            continue;
        }
        Channel channel{std::move(*transport)};

        auto direct = Device::probe(channel, kDirectIndex);
        if (direct) {
            report_device(*direct);
            continue;
        }
        if (direct.error().kind != ErrorKind::Hidpp10) {
            std::println("  no answer at index 0xFF: {}", direct.error().describe());
            continue;
        }

        // HID++ 1.0 at 0xFF => this is a receiver; paired devices live at indexes 1..6.
        std::println("  receiver (HID++ 1.0); scanning paired slots");
        for (std::uint8_t slot = 1; slot <= 6; ++slot) {
            auto dev = Device::probe(channel, slot, std::chrono::milliseconds{300});
            if (dev) report_device(*dev);
            else std::println("  [index 0x{:02X}] {}", slot, dev.error().describe());
        }
    }
    return 0;
}
