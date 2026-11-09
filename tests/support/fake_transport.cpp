#include "fake_transport.hpp"

#include <algorithm>
#include <thread>

namespace periph::test {

std::expected<void, std::string> FakeTransport::send(std::span<const std::uint8_t> frame) {
    sent_.emplace_back(frame.begin(), frame.end());
    if (responder_)
        for (auto& reply : responder_(frame)) pending_.push_back(std::move(reply));
    return {};
}

std::expected<std::size_t, std::string> FakeTransport::receive(std::span<std::uint8_t> buffer,
                                                               std::chrono::milliseconds timeout) {
    if (pending_.empty()) {
        // Behave like a real device that stays silent, without burning CPU.
        if (timeout.count() > 0) std::this_thread::sleep_for(std::min(timeout, std::chrono::milliseconds{1}));
        return std::size_t{0};
    }
    Frame frame = std::move(pending_.front());
    pending_.pop_front();
    const auto n = std::min(frame.size(), buffer.size());
    std::copy_n(frame.begin(), n, buffer.begin());
    return n;
}

Frame long_reply(std::uint8_t index, std::uint8_t feature, std::uint8_t fn_sw,
                 std::initializer_list<std::uint8_t> payload) {
    Frame f(hidpp::kLongSize, 0);
    f[0] = hidpp::kReportLong;
    f[1] = index;
    f[2] = feature;
    f[3] = fn_sw;
    std::ranges::copy(payload, f.begin() + 4);
    return f;
}

Frame hidpp20_error(std::uint8_t index, std::uint8_t feature, std::uint8_t fn_sw, std::uint8_t code) {
    return long_reply(index, 0xFF, feature, {fn_sw, code});
}

Frame hidpp10_error(std::uint8_t index, std::uint8_t sub_id, std::uint8_t address, std::uint8_t code) {
    return Frame{hidpp::kReportShort, index, 0x8F, sub_id, address, code, 0x00};
}

} // namespace periph::test
