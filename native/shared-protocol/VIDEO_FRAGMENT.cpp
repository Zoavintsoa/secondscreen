#include "VIDEO_FRAGMENT.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace second_screen::video {

namespace {
constexpr uint8_t kMagic[] = {'S', 'S', 'V', 'G'};

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

void writeU64BE(uint8_t* p, uint64_t value) {
    for (int i = 7; i >= 0; --i) {
        p[i] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

} // namespace

std::vector<std::vector<uint8_t>> fragmentAccessUnit(
    uint32_t frameId,
    uint8_t codec,
    uint8_t flags,
    uint64_t timestampUs,
    const std::vector<uint8_t>& annexB,
    uint32_t maxDatagramSize) {
    if (maxDatagramSize <= kHeaderSize ||
        annexB.empty() ||
        annexB.size() > std::numeric_limits<uint32_t>::max()) {
        return {};
    }

    const size_t payloadPerDatagram =
        static_cast<size_t>(maxDatagramSize) - kHeaderSize;
    const size_t count = (annexB.size() + payloadPerDatagram - 1) / payloadPerDatagram;

    if (count == 0 || count > std::numeric_limits<uint16_t>::max()) return {};

    std::vector<std::vector<uint8_t>> result;
    result.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const size_t offset = i * payloadPerDatagram;
        const size_t remaining = annexB.size() - offset;
        const size_t length = std::min(payloadPerDatagram, remaining);

        std::vector<uint8_t> datagram(kHeaderSize + length);
        std::copy(std::begin(kMagic), std::end(kMagic), datagram.begin());
        datagram[4] = kFragmentVersion;
        datagram[5] = codec;
        datagram[6] = flags;
        datagram[7] = 0;
        writeU32BE(datagram.data() + 8, frameId);
        writeU16BE(datagram.data() + 12, static_cast<uint16_t>(i));
        writeU16BE(datagram.data() + 14, static_cast<uint16_t>(count));
        writeU64BE(datagram.data() + 16, timestampUs);

        std::memcpy(datagram.data() + kHeaderSize, annexB.data() + offset, length);
        result.push_back(std::move(datagram));
    }

    return result;
}

} // namespace second_screen::video
