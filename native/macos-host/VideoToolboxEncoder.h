#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace secondscreen::macos {

struct EncoderCapabilities {
    bool h264 = false;
    bool hevc = false;
    bool hardwareH264 = false;
    bool hardwareHEVC = false;
};

struct EncodedAccessUnit {
    std::vector<std::uint8_t> annexB;
    std::uint64_t timestampUs = 0;
    bool keyframe = false;
};

using EncodeCallback = std::function<void(const EncodedAccessUnit&)>;

class VideoToolboxEncoder {
public:
    static EncoderCapabilities probe();

    VideoToolboxEncoder();
    ~VideoToolboxEncoder();

    bool start(int width, int height, int fps, int bitrate, const EncodeCallback& callback);
    bool encode(const void* pixelBuffer, std::uint64_t timestampUs, bool forceKeyframe);
    void stop();

private:
    struct Impl;
    Impl* impl_ = nullptr;
};

} // namespace secondscreen::macos
