#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace secondscreen::macos {

struct CaptureFrame {
    const void* pixelBuffer = nullptr;
    int width = 0;
    int height = 0;
    std::uint64_t timestampUs = 0;
};

using CaptureCallback = std::function<void(const CaptureFrame&)>;

class LegacyCapture {
public:
    virtual ~LegacyCapture() = default;

    static bool isSupported();
    static std::string backendName();

    virtual bool start(std::uint32_t displayId, CaptureCallback callback) = 0;
    virtual void stop() = 0;
};

} // namespace secondscreen::macos
