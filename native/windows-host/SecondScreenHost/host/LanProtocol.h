#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace second_screen {

enum class MessageType : uint8_t {
    Hello = 0x01, PairRequest = 0x02, PairResponse = 0x03, Capabilities = 0x04,
    StreamConfig = 0x05, Ping = 0x06, Pong = 0x07, Close = 0x08, Input = 0x09,
    Auth = 0x0A, KeyframeRequest = 0x0B, StreamPause = 0x0C, StreamResume = 0x0D, Stats = 0x0E
};

struct StreamConfig {
    uint8_t codec{1};
    uint32_t width{1920};
    uint32_t height{1080};
    uint32_t fps{60};
    uint32_t bitrateKbps{8000};
    uint32_t rotation{0};
    std::string colorSpace{"BT709"};
    uint32_t keyframeInterval{60};
    uint32_t maxLatencyMs{50};
};

std::vector<uint8_t> makeControlFrame(MessageType type, const std::string& json);
std::vector<uint8_t> makeVideoFrame(uint8_t codec, bool keyFrame, uint64_t timestampUs,
                                    const std::vector<uint8_t>& annexB);

}
