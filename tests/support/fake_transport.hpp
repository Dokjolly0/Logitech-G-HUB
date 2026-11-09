#pragma once
// In-memory ITransport: every sent frame is handed to a responder that
// returns the frames the "device" would emit in reply.

#include "periph/hidpp.hpp"

#include <deque>
#include <functional>
#include <vector>

namespace periph::test {

using Frame = std::vector<std::uint8_t>;

class FakeTransport final : public hidpp::ITransport {
public:
    using Responder = std::function<std::vector<Frame>(std::span<const std::uint8_t> request)>;

    explicit FakeTransport(Responder responder, bool long_reports = true)
        : responder_(std::move(responder)), long_reports_(long_reports) {}

    [[nodiscard]] bool supports_long_reports() const override { return long_reports_; }
    std::expected<void, std::string> send(std::span<const std::uint8_t> frame) override;
    std::expected<std::size_t, std::string> receive(std::span<std::uint8_t> buffer,
                                                    std::chrono::milliseconds timeout) override;

    // Queue unsolicited input (notifications, stale replies, other clients' traffic).
    void inject(Frame frame) { pending_.push_back(std::move(frame)); }

    [[nodiscard]] const std::vector<Frame>& sent() const { return sent_; }

private:
    Responder responder_;
    bool long_reports_;
    std::deque<Frame> pending_;
    std::vector<Frame> sent_;
};

// Builds a long-report reply: [0x11, index, feature, fn_sw, payload...] padded to 20 bytes.
Frame long_reply(std::uint8_t index, std::uint8_t feature, std::uint8_t fn_sw,
                 std::initializer_list<std::uint8_t> payload = {});
Frame hidpp20_error(std::uint8_t index, std::uint8_t feature, std::uint8_t fn_sw, std::uint8_t code);
Frame hidpp10_error(std::uint8_t index, std::uint8_t sub_id, std::uint8_t address, std::uint8_t code);

} // namespace periph::test
