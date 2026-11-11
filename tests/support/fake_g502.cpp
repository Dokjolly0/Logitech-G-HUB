#include "fake_g502.hpp"

#include "periph/device.hpp"

namespace periph::test {

using namespace hidpp;

FakeG502::FakeG502() {
    // Feature table exactly as reported by the real device (index = position).
    features = {
        {0x0000, 0x00, 0}, {0x0001, 0x00, 1}, {0x0003, 0x00, 2}, {0x0005, 0x00, 0}, {0x1D4B, 0x00, 0},
        {0x0020, 0x00, 0}, {0x1001, 0x00, 2}, {0x8070, 0x00, 4}, {0x1300, 0x00, 0}, {0x8100, 0x00, 0},
        {0x8110, 0x00, 0}, {0x8060, 0x00, 0}, {0x2201, 0x00, 1}, {0x00C2, 0x00, 0}, {0x1802, 0x60, 0},
        {0x1803, 0x60, 0}, {0x1805, 0x60, 0}, {0x1806, 0x60, 4}, {0x1811, 0x60, 0}, {0x1830, 0x60, 0},
        {0x1890, 0x60, 4}, {0x1891, 0x60, 4}, {0x18A1, 0x60, 0}, {0x1801, 0x60, 0}, {0x18B1, 0x60, 0},
        {0x1DF3, 0x60, 0}, {0x1E00, 0x40, 0}, {0x1EB0, 0x60, 0}, {0x1863, 0x60, 0}, {0x1E22, 0x60, 0},
    };
}

FakeG502 FakeG502::wireless(std::uint8_t slot) {
    FakeG502 sim;
    sim.device_index = slot;
    std::erase_if(sim.features, [](const FakeFeature& f) { return f.id == 0x00C2; });
    sim.features.push_back({0x2121, 0x00, 0});
    return sim;
}

std::uint8_t FakeG502::index_of(std::uint16_t id) const {
    for (std::size_t i = 0; i < features.size(); ++i)
        if (features[i].id == id) return static_cast<std::uint8_t>(i);
    return 0;
}

std::vector<Frame> FakeG502::handle(std::span<const std::uint8_t> rq) const {
    if (rq.size() < 4 || rq[1] != device_index) return {};
    const std::uint8_t feat = rq[2];
    const std::uint8_t fn_sw = rq[3];
    const std::uint8_t fn = fn_sw >> 4;
    auto p = [&](std::size_t i) -> std::uint8_t { return 4 + i < rq.size() ? rq[4 + i] : 0; };
    auto reply = [&](std::initializer_list<std::uint8_t> payload) {
        return std::vector<Frame>{long_reply(device_index, feat, fn_sw, payload)};
    };
    auto error = [&](std::uint8_t code) { return std::vector<Frame>{hidpp20_error(device_index, feat, fn_sw, code)}; };

    if (feat >= features.size()) return error(0x06);
    const std::uint16_t id = features[feat].id;

    switch (id) {
        case feature::Root:
            if (fn == 1 && sleeping_pings > 0) {
                --sleeping_pings;
                return {};
            }
            if (fn == 0) {
                const auto wanted = static_cast<std::uint16_t>(p(0) << 8 | p(1));
                const auto idx = index_of(wanted);
                const auto& f = features[idx];
                return reply({idx, idx ? f.type : std::uint8_t{0}, idx ? f.version : std::uint8_t{0}});
            }
            if (fn == 1) return reply({4, 2, p(2)});
            break;
        case feature::FeatureSet:
            if (fn == 0) return reply({static_cast<std::uint8_t>(features.size() - 1)});
            if (fn == 1) {
                if (p(0) >= features.size()) return error(0x02);
                const auto& f = features[p(0)];
                return reply({static_cast<std::uint8_t>(f.id >> 8), static_cast<std::uint8_t>(f.id), f.type, f.version});
            }
            break;
        case feature::DeviceNameType:
            if (fn == 0) return reply({static_cast<std::uint8_t>(name.size())});
            if (fn == 1) {
                Frame f = long_reply(device_index, feat, fn_sw);
                for (std::size_t i = 0; i < 16 && p(0) + i < name.size(); ++i)
                    f[4 + i] = static_cast<std::uint8_t>(name[p(0) + i]);
                return {f};
            }
            if (fn == 2) return reply({3}); // mouse
            break;
        case feature::BatteryVoltage:
            if (fn == 0)
                return reply({static_cast<std::uint8_t>(battery_mv >> 8), static_cast<std::uint8_t>(battery_mv),
                              battery_flags});
            break;
        case feature::AdjustableDpi:
            if (fn == 0) return reply({1});
            if (fn == 1) return reply({0, 0x00, 0x64, 0xE0, 0x32, 0x64, 0x00, 0x00, 0x00}); // 100..25600 step 50
            if (fn == 2)
                return reply({0, static_cast<std::uint8_t>(dpi >> 8), static_cast<std::uint8_t>(dpi),
                              static_cast<std::uint8_t>(default_dpi >> 8), static_cast<std::uint8_t>(default_dpi)});
            break;
        case feature::ReportRate:
            if (fn == 0) return reply({report_rate_mask});
            if (fn == 1) return reply({report_rate_ms});
            break;
        default:
            break;
    }
    return error(0x07);
}

std::unique_ptr<FakeTransport> FakeG502::transport() const {
    return std::make_unique<FakeTransport>([this](std::span<const std::uint8_t> rq) { return handle(rq); });
}

std::unique_ptr<FakeTransport> FakeG502::dj_child_transport() const {
    return std::make_unique<FakeTransport>([this](std::span<const std::uint8_t> rq) {
        Frame rewritten(rq.begin(), rq.end());
        if (rewritten.size() > 1) rewritten[1] = device_index;
        return handle(rewritten);
    });
}

std::unique_ptr<FakeTransport> fake_receiver_transport(std::uint8_t slot_error) {
    return std::make_unique<FakeTransport>([slot_error](std::span<const std::uint8_t> rq) -> std::vector<Frame> {
        const std::uint8_t code = rq[1] == kDirectIndex ? 0x01 : slot_error;
        return {hidpp10_error(rq[1], rq[2], rq[3], code)};
    });
}

} // namespace periph::test
