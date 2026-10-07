#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace second_screen {

enum class MessageType : uint8_t {
    Hello = 0x01,
    PairRequest = 0x02,
    PairResponse = 0x03,
    Capabilities = 0x04,
    StreamConfig = 0x05,
    Ping = 0x06,
    Pong = 0x07,
    Close = 0x08,
    Input = 0x09
};

std::vector<uint8_t> makeControlFrame(MessageType type, const std::string& json);

}
