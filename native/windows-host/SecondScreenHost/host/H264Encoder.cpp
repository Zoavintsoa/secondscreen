#include "H264Encoder.h"

namespace second_screen {

bool H264Encoder::initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrateKbps) {
    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrateKbps_ = bitrateKbps;
    // Media Foundation MFT initialization belongs here. The production path
    // selects a hardware H.264 encoder first and falls back to software.
    return width_ > 0 && height_ > 0 && fps_ > 0 && bitrateKbps_ > 0;
}

bool H264Encoder::encode(void* d3dTexture, const FrameInfo& info, EncodedAccessUnit& output) {
    if (!d3dTexture) {
        return false;
    }

    // Intentionally no fake encoded bytes are returned. Until the Media
    // Foundation MFT is connected, encode() reports failure rather than
    // pretending that a simulation is a real stream.
    (void)info;
    (void)output;
    return false;
}

void H264Encoder::requestKeyFrame() {
    forceKeyFrame_ = true;
}

void H264Encoder::shutdown() {
    width_ = height_ = fps_ = bitrateKbps_ = 0;
    forceKeyFrame_ = false;
}

}
