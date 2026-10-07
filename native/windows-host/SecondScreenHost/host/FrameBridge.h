#pragma once

#include <cstdint>
#include <functional>

namespace second_screen {

struct FrameInfo {
    uint32_t width{};
    uint32_t height{};
    uint64_t timestampUs{};
    bool keyFrame{};
};

using FrameCallback = std::function<void(const FrameInfo&)>;

class FrameBridge {
public:
    bool start(FrameCallback callback);
    void stop();

    // Called by the IddCx integration after a frame is acquired.
    // The production implementation must keep the frame GPU-backed until
    // the encoder has consumed it.
    bool submitGpuFrame(void* d3dTexture, const FrameInfo& info);

private:
    FrameCallback callback_;
};

}
