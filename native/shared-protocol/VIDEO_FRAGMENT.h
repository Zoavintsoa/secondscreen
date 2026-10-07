#pragma once

#include <cstdint>
#include <vector>

namespace second_screen::video {

inline constexpr uint8_t kFragmentVersion = 1;
inline constexpr uint32_t kHeaderSize = 24;

struct Fragment {
    uint32_t frameId{};
    uint16_t index{};
    uint16_t count{};
    uint8_t codec{};
    uint8_t flags{};
    uint64_t timestampUs{};
    std::vector<uint8_t> payload;
};

std::vector<std::vector<uint8_t>> fragmentAccessUnit(
    uint32_t frameId,
    uint8_t codec,
    uint8_t flags,
    uint64_t timestampUs,
    const std::vector<uint8_t>& annexB,
    uint32_t maxDatagramSize);

} // namespace second_screen::video
