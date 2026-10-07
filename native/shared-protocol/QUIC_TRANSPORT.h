#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>
#include <string>

namespace second_screen::quic {

struct DatagramLimits {
    uint16_t maxSendLength{1200};
};

struct TransportCallbacks {
    std::function<void(const std::vector<uint8_t>&)> onControl;
    std::function<void(const std::vector<uint8_t>&)> onVideoDatagram;
    std::function<void(const std::string&)> onClosed;

    std::function<void(bool)> onAuthenticated;
    std::function<void()> onKeyframeRequested;
    std::function<void(const std::string&, const std::string&)> onPairingChallenge;

    std::function<bool(const std::string&, const std::string&)> validateSessionToken;
    std::function<bool(const std::string&, const std::string&)> confirmPairingCode;
    std::function<std::string(const std::string&)> issueSessionToken;
};

class IQuicTransport {
public:
    virtual ~IQuicTransport() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool sendControl(const uint8_t*, size_t) = 0;
    virtual bool sendVideoDatagram(const uint8_t*, size_t) = 0;
    virtual bool connected() const = 0;
    virtual DatagramLimits datagramLimits() const = 0;
};

}
