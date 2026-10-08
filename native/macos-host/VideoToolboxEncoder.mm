#import <VideoToolbox/VideoToolbox.h>
#import <CoreMedia/CoreMedia.h>

#include "VideoToolboxEncoder.h"

namespace secondscreen::macos {

VideoToolboxEncoder::VideoToolboxEncoder() = default;
VideoToolboxEncoder::~VideoToolboxEncoder() { stop(); }

EncoderCapabilities VideoToolboxEncoder::probe() {
    EncoderCapabilities caps;

    // VideoToolbox is available on supported macOS releases, but this probe
    // must not infer "hardware" merely from a property dictionary. Hardware
    // acceptance requires a real VTCompressionSession and hardware-path test.
    caps.h264 = true;

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
    CFDictionaryRef properties = nullptr;
    if (VTCopySupportedPropertyDictionaryForEncoder(
            1920, 1080, kCMVideoCodecType_HEVC, nullptr, nullptr, &properties) == noErr) {
        caps.hevc = properties != nullptr;
        if (properties) CFRelease(properties);
    }
#endif

    caps.hardwareH264 = false;
    caps.hardwareHEVC = false;
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
