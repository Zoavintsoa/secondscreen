#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <d3d11.h>

#include "DiscoveryService.h"
#include "PairingManager.h"
#include "ControlServer.h"
#include "FrameBridge.h"
#include "H264Encoder.h"
#include "VideoStreamServer.h"
#include "DriverFrameReceiver.h"
#include "MsQuicServer.h"

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
    void onClientAuth(bool authenticated);
    void requestKeyframe();
    void onQuicAuth(bool authenticated);

    std::atomic_bool running_{false};
    DiscoveryService discovery_;
    PairingManager pairing_;
    std::unique_ptr<ControlServer> controlServer_;
    FrameBridge frameBridge_;
    H264Encoder encoder_;
    VideoStreamServer videoStream_;
    DriverFrameReceiver frameReceiver_;
    std::unique_ptr<MsQuicServer> quicServer_;
    bool quicAuthenticated_{false};
    uint32_t quicFrameId_{0};
    std::mutex pipelineMutex_;
    uint32_t encoderWidth_{};
    uint32_t encoderHeight_{};
    uint32_t encoderFps_{};
};

}
