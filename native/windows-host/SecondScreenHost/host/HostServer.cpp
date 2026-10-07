#include "HostServer.h"
#include <winsock2.h>
#include <ws2tcpip.h>

namespace second_screen {

namespace {
constexpr uint16_t kControlPort = 49152;
constexpr uint16_t kVideoBringUpPort = 49153;
}

HostServer::HostServer() = default;
HostServer::~HostServer() { stop(); }

bool HostServer::start() {
    if (running_.exchange(true)) return true;

    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        running_ = false;
        return false;
    }

    if (!discovery_.start("SecondScreen Host", kControlPort, kVideoBringUpPort)) {
        WSACleanup();
        running_ = false;
        return false;
    }

    if (!videoStream_.start(kVideoBringUpPort)) {
        discovery_.stop();
        WSACleanup();
        running_ = false;
        return false;
    }

    if (!frameBridge_.start([this](ID3D11Texture2D* texture, const FrameInfo& info) {
        onFrame(texture, info);
    })) {
        videoStream_.stop();
        discovery_.stop();
        WSACleanup();
        running_ = false;
        return false;
    }

    if (!frameReceiver_.start(this)) {
        frameBridge_.stop();
        videoStream_.stop();
        discovery_.stop();
        WSACleanup();
        running_ = false;
        return false;
    }

    return true;
}

void HostServer::stop() {
    if (!running_.exchange(false)) return;
    frameReceiver_.stop();
    frameBridge_.stop();
    encoder_.shutdown();
    videoStream_.stop();
    discovery_.stop();
    controlThread_.reset();
    discoveryThread_.reset();
    WSACleanup();
}

bool HostServer::submitFrame(ID3D11Texture2D* texture, const FrameInfo& info) {
    if (!running_ || !texture) return false;
    return frameBridge_.submitGpuFrame(texture, info);
}

void HostServer::onFrame(ID3D11Texture2D* texture, const FrameInfo& info) {
    if (!videoStream_.hasClient()) return;

    std::lock_guard lock(pipelineMutex_);

    if (encoderWidth_ != info.width || encoderHeight_ != info.height || encoderFps_ == 0) {
        encoder_.shutdown();
        encoderWidth_ = info.width;
        encoderHeight_ = info.height;
        encoderFps_ = 60;

        if (!encoder_.initialize(encoderWidth_, encoderHeight_, encoderFps_, 8000)) {
            encoderWidth_ = encoderHeight_ = encoderFps_ = 0;
            return;
        }
        encoder_.requestKeyFrame();
    }

    EncodedAccessUnit accessUnit;
    if (!encoder_.encode(texture, info, accessUnit)) return;

    const auto packet = makeVideoFrame(
        1,
        static_cast<uint8_t>(accessUnit.keyFrame ? 0x01 : 0x00),
        accessUnit.timestampUs ? accessUnit.timestampUs : info.timestampUs,
        accessUnit.annexB);

    videoStream_.sendFrame(packet);
}

}
