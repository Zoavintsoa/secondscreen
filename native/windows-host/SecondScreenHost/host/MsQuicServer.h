#pragma once
#include "../../../shared-protocol/QUIC_TRANSPORT.h"
#include <memory>
#include <string>

namespace second_screen {

class MsQuicServer final : public quic::IQuicTransport {
public:
    struct Config {
        uint16_t port{49152};
        std::string certificateThumbprint;
    };

    MsQuicServer(Config config, quic::TransportCallbacks callbacks);
    ~MsQuicServer() override;

    bool start() override;
    void stop() override;
    bool sendControl(const uint8_t*, size_t) override;
    bool sendVideoDatagram(const uint8_t*, size_t) override;
    bool connected() const override;
    quic::DatagramLimits datagramLimits() const override;

public:
    // Public only so the callback translation unit can legally name the
    // private implementation type; callers still interact through IQuicTransport.
    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

}
