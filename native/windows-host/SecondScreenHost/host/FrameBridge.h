#pragma once

#include <cstdint>
#include <functional>
#include <d3d11.h>

namespace second_screen {

struct FrameInfo {
    uint32_t width{};
    uint32_t height{};
    uint64_t timestampUs{};
    bool keyFrame{};
};

using FrameCallback = std::function<void(ID3D11Texture2D*, const FrameInfo&)>;

class FrameBridge {
public:
    bool start(FrameCallback callback);
    void stop();
    bool submitGpuFrame(ID3D11Texture2D* d3dTexture, const FrameInfo& info);

private:
    FrameCallback callback_;
};

}
