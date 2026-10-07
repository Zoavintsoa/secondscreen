#include "HostServer.h"
#include "../../../shared-protocol/VIDEO_FRAGMENT.h"
#include <cstdlib>
#include <iostream>
#include <windows.h>
#include <winsock2.h>

namespace second_screen {
namespace {
constexpr uint16_t kControlPort=49152;
constexpr uint16_t kVideoBringUpPort=49153;
std::string readQuicCertificateThumbprint() {
    char buffer[256]{};
    const DWORD length = GetEnvironmentVariableA(
        "SECOND_SCREEN_QUIC_CERT_THUMBPRINT",
        buffer,
        static_cast<DWORD>(sizeof(buffer)));
    if (length == 0 || length >= sizeof(buffer)) return {};
    return std::string(buffer, length);
}
}

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

    const std::string thumbprint = readQuicCertificateThumbprint();
    if (!thumbprint.empty()) {
        quic::TransportCallbacks callbacks;
        callbacks.onAuthenticated = [this](bool authenticated) { onQuicAuth(authenticated); };
        callbacks.onKeyframeRequested = [this]() { requestKeyframe(); };
        callbacks.createPairingChallenge = [this](const std::string& deviceId) {
            return pairing_.createChallenge(deviceId).code;
        };
        callbacks.onPairingChallenge = [](const std::string& deviceId, const std::string& code) {
            std::cout << "[SecondScreen] QUIC pairing code for " << deviceId
                      << ": " << code << " (valid 120s)" << std::endl;
        };
        callbacks.validateSessionToken = [this](const std::string& deviceId, const std::string& token) {
            return pairing_.validateSessionToken(deviceId, token);
        };
        callbacks.confirmPairingCode = [this](const std::string& deviceId, const std::string& code) {
            return pairing_.confirm(deviceId, code);
        };
        callbacks.issueSessionToken = [this](const std::string& deviceId) {
            return pairing_.issueSessionToken(deviceId);
        };

        quicServer_ = std::make_unique<MsQuicServer>(
            MsQuicServer::Config{kControlPort, thumbprint},
            std::move(callbacks));

        if (!quicServer_->start()) {
            std::cerr << "[SecondScreen] MsQuic start failed; TCP bring-up remains available.\n";
            quicServer_.reset();
        }
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
    if (quicServer_) {
        quicServer_->stop();
        quicServer_.reset();
    }
    quicAuthenticated_ = false;
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

void HostServer::onQuicAuth(bool authenticated) {
    quicAuthenticated_ = authenticated;
    if (authenticated) {
        std::lock_guard lock(pipelineMutex_);
        encoder_.requestKeyFrame();
    }
}

void HostServer::requestKeyframe() {
    std::lock_guard lock(pipelineMutex_);
    encoder_.requestKeyFrame();
}

void HostServer::onFrame(ID3D11Texture2D* texture, const FrameInfo& info) {
    const bool quicReady =
        quicServer_ &&
        quicServer_->connected() &&
        quicAuthenticated_;
    const bool tcpReady =
        videoStream_.hasClient() &&
        videoStream_.isAuthorized();

    if (!quicReady && !tcpReady) return;
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
    const uint64_t timestamp =
        accessUnit.timestampUs ? accessUnit.timestampUs : info.timestampUs;

    if (quicReady) {
        const uint16_t maxDatagram =
            quicServer_->datagramLimits().maxSendLength;
        if (maxDatagram > 32) {
            const uint8_t flags = accessUnit.keyFrame ? 0x01 : 0x00;
            const auto fragments = video::fragmentAccessUnit(
                ++quicFrameId_,
                video::Codec::H264,
                flags,
                timestamp,
                accessUnit.annexB,
                maxDatagram);

            bool allSent = !fragments.empty();
            for (const auto& fragment : fragments) {
                if (!quicServer_->sendVideoDatagram(
                        fragment.data(), fragment.size())) {
                    allSent = false;
                    break;
                }
            }
            if (!allSent) encoder_.requestKeyFrame();
        }
    } else if (tcpReady) {
        videoStream_.sendFrame(
            accessUnit.annexB,
            accessUnit.keyFrame,
            timestamp);
    }
}
}
