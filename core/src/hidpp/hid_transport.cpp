#include "periph/hid_transport.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <regex>

namespace periph::hidpp {
namespace {

class HidTransport final : public ITransport {
public:
    struct Handle {
        hid::Device device;
        std::uint16_t usage{};
    };

    explicit HidTransport(std::vector<Handle> handles) : handles_(std::move(handles)) {
        auto it = std::ranges::find(handles_, kUsageLong, &Handle::usage);
        has_long_ = it != handles_.end();
        writer_ = has_long_ ? static_cast<std::size_t>(it - handles_.begin()) : 0;
    }

    [[nodiscard]] bool supports_long_reports() const override { return has_long_; }

    std::expected<void, std::string> send(std::span<const std::uint8_t> frame) override {
        return handles_[writer_].device.write(frame);
    }

    std::expected<std::size_t, std::string> receive(std::span<std::uint8_t> buffer,
                                                    std::chrono::milliseconds timeout) override {
        if (handles_.size() == 1) return handles_.front().device.read(buffer, timeout);

        // Several collections: poll each in short slices until one has data.
        using clock = std::chrono::steady_clock;
        const auto deadline = clock::now() + timeout;
        const auto slice = timeout.count() == 0 ? std::chrono::milliseconds{0} : std::chrono::milliseconds{5};
        do {
            for (auto& h : handles_) {
                auto n = h.device.read(buffer, slice);
                if (!n || *n > 0) return n;
            }
        } while (clock::now() < deadline);
        return std::size_t{0};
    }

private:
    std::vector<Handle> handles_;
    std::size_t writer_{};
    bool has_long_{};
};

} // namespace

namespace detail {
std::string endpoint_key(const std::string& path) {
    std::string key = path;
    std::ranges::transform(key, key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    key = std::regex_replace(key, std::regex{"&col[0-9a-f]{2}"}, "");
    key = std::regex_replace(key, std::regex{"&[0-9a-f]{4}#\\{"}, "#{");
    return key;
}
} // namespace detail

std::vector<Endpoint> group_endpoints(std::vector<hid::DeviceInfo> infos) {
    std::map<std::string, Endpoint> grouped;
    for (auto& info : infos) {
        if (info.usage_page != kUsagePage) continue;
        if (info.usage != kUsageShort && info.usage != kUsageLong) continue;
        auto key = detail::endpoint_key(info.path);
        auto& ep = grouped[key];
        if (ep.collections.empty()) {
            ep.key = key;
            ep.product_id = info.product_id;
            ep.product = info.product;
            if (info.receiver_slot) ep.device_index = *info.receiver_slot;
        }
        // Linux/macOS report the same path once per usage: open it only once,
        // but remember that long reports are available.
        auto same = std::ranges::find(ep.collections, info.path, &hid::DeviceInfo::path);
        if (same == ep.collections.end()) {
            ep.collections.push_back(std::move(info));
        } else if (info.usage == kUsageLong) {
            same->usage = kUsageLong;
        }
    }
    std::vector<Endpoint> result;
    for (auto& [_, ep] : grouped) result.push_back(std::move(ep));
    return result;
}

std::vector<Endpoint> find_endpoints() { return group_endpoints(hid::enumerate(kLogitechVendorId)); }

std::expected<std::unique_ptr<ITransport>, std::string> open_hid_transport(const Endpoint& endpoint) {
    std::vector<HidTransport::Handle> handles;
    for (const auto& c : endpoint.collections) {
        auto dev = hid::Device::open(c.path);
        if (!dev) return std::unexpected(dev.error());
        handles.push_back({std::move(*dev), c.usage});
    }
    if (handles.empty()) return std::unexpected(std::string{"no HID++ collection"});
    return std::make_unique<HidTransport>(std::move(handles));
}

} // namespace periph::hidpp
