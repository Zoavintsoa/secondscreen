#import <VideoToolbox/VideoToolbox.h>
#import <CoreMedia/CoreMedia.h>

#include "VideoToolboxEncoder.h"

namespace secondscreen::macos {

VideoToolboxEncoder::VideoToolboxEncoder() = default;
VideoToolboxEncoder::~VideoToolboxEncoder() { stop(); }

EncoderCapabilities VideoToolboxEncoder::probe() {
    EncoderCapabilities caps;
    caps.h264 = true;
    caps.hevc = false;

    CFDictionaryRef properties = nullptr;
    if (VTCopySupportedPropertyDictionaryForEncoder(
            1920, 1080, kCMVideoCodecType_H264, nullptr, &properties) == noErr) {
        caps.hardwareH264 = properties != nullptr;
        if (properties) CFRelease(properties);
    }

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
    if (VTCopySupportedPropertyDictionaryForEncoder(
            1920, 1080, kCMVideoCodecType_HEVC, nullptr, &properties) == noErr) {
        caps.hevc = properties != nullptr;
        if (properties) CFRelease(properties);
    }
#endif
    return caps;
}

bool VideoToolboxEncoder::start(int, int, int, int, const EncodeCallback&) {
    return false;
}

bool VideoToolboxEncoder::encode(const void*, std::uint64_t, bool) {
    return false;
}

void VideoToolboxEncoder::stop() {}

} // namespace secondscreen::macos
