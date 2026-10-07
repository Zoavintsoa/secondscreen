#pragma once

#include <cstdint>
#include <vector>
#include "FrameBridge.h"

namespace second_screen {

struct EncodedAccessUnit {
    std::vector<uint8_t> annexB;
    uint64_t timestampUs{};
    bool keyFrame{};
};

class H264Encoder {
public:
    bool initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrateKbps);
    bool encode(void* d3dTexture, const FrameInfo& info, EncodedAccessUnit& output);
    void requestKeyFrame();
    void shutdown();

private:
    bool forceKeyFrame_{false};
    uint32_t width_{};
    uint32_t height_{};
    uint32_t fps_{};
    uint32_t bitrateKbps_{};
};

}
