#include "ControlServer.h"

#include "../../../shared-protocol/CONTROL_PROTOCOL.h"
#include "../../../shared-protocol/CONTROL_SESSION.h"

#include <iostream>
#include <string>
#include <vector>

namespace second_screen {

using second_screen::control::ControlFrameParser;
using second_screen::control::ControlMessage;
using second_screen::control::ControlSession;
using second_screen::control::ParseStatus;
using second_screen::control::makeControlFrame;

ControlServer::ControlServer(
    PairingManager& pairing,
    AuthCallback authCallback,
    KeyframeCallback keyframeCallback)
    : pairing_(pairing),
      authCallback_(std::move(authCallback)),
      keyframeCallback_(std::move(keyframeCallback)) {}

ControlServer::~ControlServer() {
    stop();
}

bool ControlServer::start(uint16_t port) {
    if (running_.exchange(true)) return true;

    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
        running_ = false;
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSocket_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
        listen(listenSocket_, 1) == SOCKET_ERROR) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
        running_ = false;
        return false;
    }

    acceptThread_ = std::make_unique<std::thread>(&ControlServer::acceptLoop, this);
    return true;
}

void ControlServer::acceptLoop() {
    while (running_) {
        SOCKET client = accept(listenSocket_, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            if (running_) continue;
            break;
        }

        BOOL noDelay = TRUE;
        setsockopt(client, IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

        {
            std::lock_guard lock(clientMutex_);
            if (clientSocket_ != INVALID_SOCKET) closesocket(clientSocket_);
            clientSocket_ = client;
        }

        clientLoop(client);

        {
            std::lock_guard lock(clientMutex_);
            if (clientSocket_ == client) {
                closesocket(clientSocket_);
                clientSocket_ = INVALID_SOCKET;
            }
        }
    }
}

void ControlServer::clientLoop(SOCKET client) {
    ControlSession::SecurityCallbacks security;
    security.validateSessionToken =
        [this](const std::string& deviceId, const std::string& token) {
            return pairing_.validateSessionToken(deviceId, token);
        };
    security.confirmPairingCode =
        [this](const std::string& deviceId, const std::string& code) {
            return pairing_.confirm(deviceId, code);
        };
    security.issueSessionToken =
        [this](const std::string& deviceId) {
            return pairing_.issueSessionToken(deviceId);
        };

    ControlSession session(std::move(security));
    std::vector<uint8_t> buffer(4096);
    ControlFrameParser parser;

    auto sendAction = [client](const second_screen::control::SessionAction& action) {
        if (action.responseJson.empty()) return true;
        const auto frame = makeControlFrame(action.responseType, action.responseJson);
        if (frame.empty()) return false;

        size_t sentTotal = 0;
        while (sentTotal < frame.size()) {
            const int sent = send(
                client,
                reinterpret_cast<const char*>(frame.data() + sentTotal),
                static_cast<int>(frame.size() - sentTotal),
                0);
            if (sent <= 0) return false;
            sentTotal += static_cast<size_t>(sent);
        }
        return true;
    };

    bool authenticated = false;

    while (running_) {
        const int received = recv(client, reinterpret_cast<char*>(buffer.data()),
                                  static_cast<int>(buffer.size()), 0);
        if (received <= 0) break;

        ControlMessage message;
        const auto status = parser.push(buffer.data(), static_cast<size_t>(received), message);
        if (status == ParseStatus::Invalid) break;
        if (status == ParseStatus::NeedMoreData) continue;

        const auto action = session.onMessage(message);
        if (!sendAction(action)) break;

        if (action.authenticated && !authenticated) {
            authenticated = true;
            if (authCallback_) authCallback_(true);
        }
        if (action.requestKeyframe && keyframeCallback_) keyframeCallback_();
        if (action.close) break;
    }

    if (authenticated && authCallback_) authCallback_(false);
}

}
