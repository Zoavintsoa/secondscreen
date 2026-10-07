#include "CONTROL_PROTOCOL.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace second_screen::control {

namespace {
constexpr uint8_t kMagic[] = {'S', 'S', 'C', 'P'};
constexpr size_t kHeaderSize = 12;

uint16_t readU16BE(const uint8_t* p) {
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

uint32_t readU32BE(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) |
           static_cast<uint32_t>(p[3]);
}

void writeU16BE(uint8_t* p, uint16_t value) {
    p[0] = static_cast<uint8_t>(value >> 8);
    p[1] = static_cast<uint8_t>(value);
}

void writeU32BE(uint8_t* p, uint32_t value) {
    p[0] = static_cast<uint8_t>(value >> 24);
    p[1] = static_cast<uint8_t>(value >> 16);
    p[2] = static_cast<uint8_t>(value >> 8);
    p[3] = static_cast<uint8_t>(value);
}

} // namespace

ControlFrameParser::ControlFrameParser(uint32_t maxPayloadBytes)
    : maxPayloadBytes_(std::max<uint32_t>(1, maxPayloadBytes)) {
    buffer_.reserve(kHeaderSize);
}

void ControlFrameParser::reset() {
    buffer_.clear();
    invalid_ = false;
}

ParseStatus ControlFrameParser::push(
    const uint8_t* data,
    size_t size,
    ControlMessage& out) {
    if (invalid_) return ParseStatus::Invalid;
    if (size != 0 && data == nullptr) {
        invalid_ = true;
        return ParseStatus::Invalid;
    }

    if (size > std::numeric_limits<size_t>::max() - buffer_.size()) {
        invalid_ = true;
        return ParseStatus::Invalid;
    }

    buffer_.insert(buffer_.end(), data, data + size);

    if (buffer_.size() < kHeaderSize) return ParseStatus::NeedMoreData;

    if (!std::equal(std::begin(kMagic), std::end(kMagic), buffer_.begin())) {
        invalid_ = true;
        return ParseStatus::Invalid;
    }

    const uint8_t major = buffer_[4];
    const uint8_t type = buffer_[5];
    const uint16_t flags = readU16BE(buffer_.data() + 6);
    const uint32_t payloadLength = readU32BE(buffer_.data() + 8);

    if (major != kProtocolMajor || !isKnownMessageType(type)) {
        invalid_ = true;
        return ParseStatus::Invalid;
    }

    if (payloadLength > maxPayloadBytes_) {
        invalid_ = true;
        return ParseStatus::Invalid;
    }

    const size_t total = kHeaderSize + static_cast<size_t>(payloadLength);
    if (buffer_.size() < total) return ParseStatus::NeedMoreData;

    out.major = major;
    out.type = static_cast<MessageType>(type);
    out.flags = flags;
    out.json.assign(
        reinterpret_cast<const char*>(buffer_.data() + kHeaderSize),
        payloadLength);

    buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(total));
    return ParseStatus::MessageReady;
}

ParseStatus ControlFrameParser::push(std::string_view bytes, ControlMessage& out) {
    return push(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), out);
}

std::vector<uint8_t> makeControlFrame(
    MessageType type,
    std::string_view json,
    uint16_t flags,
    uint8_t major) {
    if (major != kProtocolMajor ||
        !isKnownMessageType(static_cast<uint8_t>(type)) ||
        json.size() > std::numeric_limits<uint32_t>::max()) {
        return {};
    }

    const size_t total = kHeaderSize + json.size();
    std::vector<uint8_t> frame(total);
    std::copy(std::begin(kMagic), std::end(kMagic), frame.begin());
    frame[4] = major;
    frame[5] = static_cast<uint8_t>(type);
    writeU16BE(frame.data() + 6, flags);
    writeU32BE(frame.data() + 8, static_cast<uint32_t>(json.size()));

    if (!json.empty()) {
        std::memcpy(frame.data() + kHeaderSize, json.data(), json.size());
    }
    return frame;
}

bool isKnownMessageType(uint8_t type) noexcept {
    return type >= static_cast<uint8_t>(MessageType::Hello) &&
           type <= static_cast<uint8_t>(MessageType::Stats);
}

} // namespace second_screen::control
