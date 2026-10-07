#include "FrameBridge.h"

namespace second_screen {

bool FrameBridge::start(FrameCallback callback) {
    callback_ = std::move(callback);
    return true;
}

void FrameBridge::stop() {
    callback_ = nullptr;
}

bool FrameBridge::submitGpuFrame(void* d3dTexture, const FrameInfo& info) {
    if (!d3dTexture || !callback_) {
        return false;
    }

    callback_(info);
    return true;
}

}
