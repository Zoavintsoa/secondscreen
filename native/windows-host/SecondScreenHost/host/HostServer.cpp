#include "HostServer.h"
#include <winsock2.h>

namespace second_screen {
namespace { constexpr uint16_t kControlPort=49152; constexpr uint16_t kVideoBringUpPort=49153; }

HostServer::HostServer() = default;
HostServer::~HostServer() { stop(); }

bool HostServer::start() {
    if (running_.exchange(true)) return true;
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2,2), &data) != 0) { running_=false; return false; }

    if (!discovery_.start("SecondScreen Host", kControlPort, kVideoBringUpPort)) {
        WSACleanup(); running_=false; return false;
    }

    controlServer_ = std::make_unique<ControlServer>(
        pairing_,
        [this](bool authenticated){ onClientAuth(authenticated); },
        [this](){ requestKeyframe(); });

    if (!controlServer_->start(kControlPort)) {
        controlServer_.reset(); discovery_.stop(); WSACleanup(); running_=false; return false;
    }

    if (!videoStream_.start(kVideoBringUpPort)) {
        controlServer_->stop(); controlServer_.reset(); discovery_.stop(); WSACleanup(); running_=false; return false;
    }

    if (!frameBridge_.start([this](ID3D11Texture2D* texture, const FrameInfo& info){ onFrame(texture, info); })) {
        videoStream_.stop(); controlServer_->stop(); controlServer_.reset(); discovery_.stop(); WSACleanup(); running_=false; return false;
    }

    if (!frameReceiver_.start(this)) {
        frameBridge_.stop(); videoStream_.stop(); controlServer_->stop(); controlServer_.reset(); discovery_.stop(); WSACleanup(); running_=false; return false;
    }
    return true;
}

void HostServer::stop() {
    if (!running_.exchange(false)) return;
    frameReceiver_.stop();
    frameBridge_.stop();
    encoder_.shutdown();
    videoStream_.stop();
    if (controlServer_) { controlServer_->stop(); controlServer_.reset(); }
    discovery_.stop();
    WSACleanup();
}

bool HostServer::submitFrame(ID3D11Texture2D* texture, const FrameInfo& info) {
    if (!running_ || !texture) return false;
    return frameBridge_.submitGpuFrame(texture, info);
}

void HostServer::onClientAuth(bool authenticated) {
    videoStream_.setAuthorized(authenticated);
    if (!authenticated) {
        std::lock_guard lock(pipelineMutex_);
        encoder_.requestKeyFrame();
    }
}

void HostServer::requestKeyframe() {
    std::lock_guard lock(pipelineMutex_);
    encoder_.requestKeyFrame();
}

void HostServer::onFrame(ID3D11Texture2D* texture, const FrameInfo& info) {
    if (!videoStream_.hasClient()) return;
    std::lock_guard lock(pipelineMutex_);

    if (encoderWidth_ != info.width || encoderHeight_ != info.height || encoderFps_ == 0) {
        encoder_.shutdown();
        encoderWidth_=info.width; encoderHeight_=info.height; encoderFps_=60;
        if (!encoder_.initialize(encoderWidth_, encoderHeight_, encoderFps_, 8000)) {
            encoderWidth_=encoderHeight_=encoderFps_=0; return;
        }
        encoder_.requestKeyFrame();
    }

    EncodedAccessUnit accessUnit;
    if (!encoder_.encode(texture, info, accessUnit)) return;
    videoStream_.sendFrame(accessUnit.annexB, accessUnit.keyFrame, accessUnit.timestampUs ? accessUnit.timestampUs : info.timestampUs);
}
}
