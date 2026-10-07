#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace second_screen::control {

inline constexpr uint8_t kProtocolMajor = 1;
inline constexpr uint32_t kDefaultMaxPayloadBytes = 64u * 1024u;

enum class MessageType : uint8_t {
    Hello = 0x01,
    PairRequest = 0x02,
    PairResponse = 0x03,
    Capabilities = 0x04,
    StreamConfig = 0x05,
    Ping = 0x06,
    Pong = 0x07,
    Close = 0x08,
    Input = 0x09,
    Auth = 0x0A,
    KeyframeRequest = 0x0B,
    StreamPause = 0x0C,
    StreamResume = 0x0D,
    Stats = 0x0E
};

struct ControlMessage {
    MessageType type{};
    uint8_t major{};
    uint16_t flags{};
    std::string json;
};

enum class ParseStatus {
    NeedMoreData,
    MessageReady,
    Invalid
};

class ControlFrameParser {
public:
    explicit ControlFrameParser(uint32_t maxPayloadBytes = kDefaultMaxPayloadBytes);

    void reset();
    ParseStatus push(const uint8_t* data, size_t size, ControlMessage& out);
    ParseStatus push(std::string_view bytes, ControlMessage& out);

    bool invalid() const noexcept { return invalid_; }
    size_t bufferedBytes() const noexcept { return buffer_.size(); }

private:
    uint32_t maxPayloadBytes_;
    bool invalid_{false};
    std::vector<uint8_t> buffer_;
};

std::vector<uint8_t> makeControlFrame(
    MessageType type,
    std::string_view json,
    uint16_t flags = 0,
    uint8_t major = kProtocolMajor);

bool isKnownMessageType(uint8_t type) noexcept;

} // namespace second_screen::control
