#pragma once

#include <cstdint>
#include <vector>

namespace second_screen::transport {

struct SessionId {
    uint64_t value{};
};

class IControlChannel {
public:
    virtual ~IControlChannel() = default;
    virtual bool send(const std::vector<uint8_t>& message) = 0;
    virtual bool isConnected() const = 0;
};

class IVideoChannel {
public:
    virtual ~IVideoChannel() = default;
    virtual bool sendDatagram(const std::vector<uint8_t>& accessUnit) = 0;
    virtual bool isConnected() const = 0;
};

class ITransportSession {
public:
    virtual ~ITransportSession() = default;
    virtual IControlChannel* control() = 0;
    virtual IVideoChannel* video() = 0;
    virtual void close() = 0;
};

}
