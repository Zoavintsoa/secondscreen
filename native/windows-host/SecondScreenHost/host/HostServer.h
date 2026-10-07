#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <d3d11.h>

#include "DiscoveryService.h"
#include "PairingManager.h"
#include "FrameBridge.h"
#include "H264Encoder.h"
#include "LanProtocol.h"
#include "VideoStreamServer.h"

namespace second_screen {

class HostServer {
public:
    HostServer();
    ~HostServer();

    bool start();
    void stop();
    bool submitFrame(ID3D11Texture2D* texture, const FrameInfo& info);

private:
    void onFrame(ID3D11Texture2D* texture, const FrameInfo& info);

    std::atomic_bool running_{false};
    std::unique_ptr<std::thread> discoveryThread_;
    std::unique_ptr<std::thread> controlThread_;
    DiscoveryService discovery_;
    FrameBridge frameBridge_;
    H264Encoder encoder_;
    VideoStreamServer videoStream_;
    std::mutex pipelineMutex_;
    uint32_t encoderWidth_{};
    uint32_t encoderHeight_{};
    uint32_t encoderFps_{};
};

}
