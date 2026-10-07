#include "LanProtocol.h"

namespace second_screen {

static void put32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>(value & 0xff));
}

std::vector<uint8_t> makeControlFrame(MessageType type, const std::string& json) {
    std::vector<uint8_t> out;
    out.reserve(12 + json.size());
    out.insert(out.end(), {'S','S','C','P'});
    out.push_back(1);
    out.push_back(static_cast<uint8_t>(type));
    out.push_back(0);
    out.push_back(0);
    put32(out, static_cast<uint32_t>(json.size()));
    out.insert(out.end(), json.begin(), json.end());
    return out;
}

}
