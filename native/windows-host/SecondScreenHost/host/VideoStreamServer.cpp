#include "VideoStreamServer.h"

#include <algorithm>

namespace second_screen {

VideoStreamServer::~VideoStreamServer() {
    stop();
}

bool VideoStreamServer::start(uint16_t port) {
    if (running_.exchange(true)) return true;

    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
        running_ = false;
        return false;
    }

    BOOL noDelay = TRUE;
    setsockopt(listenSocket_, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

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

    acceptThread_ = std::make_unique<std::thread>(&VideoStreamServer::acceptLoop, this);
    return true;
}

void VideoStreamServer::acceptLoop() {
    while (running_) {
        SOCKET s = accept(listenSocket_, nullptr, nullptr);
        if (s == INVALID_SOCKET) {
            if (running_) continue;
            break;
        }

        BOOL noDelay = TRUE;
        setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

        {
            std::lock_guard lock(socketMutex_);
            if (clientSocket_ != INVALID_SOCKET) closesocket(clientSocket_);
            clientSocket_ = s;
            clientConnected_ = true;
        }
    }
}

bool VideoStreamServer::hasClient() const {
    return clientConnected_.load();
}

void VideoStreamServer::sendFrame(const std::vector<uint8_t>& frame) {
    if (!running_ || frame.empty()) return;

    std::lock_guard lock(socketMutex_);
    if (clientSocket_ == INVALID_SOCKET) {
        clientConnected_ = false;
        return;
    }

    size_t sentTotal = 0;
    while (sentTotal < frame.size()) {
        const int chunk = static_cast<int>(
            std::min<size_t>(frame.size() - sentTotal, static_cast<size_t>(INT_MAX)));
        const int sent = send(clientSocket_,
                              reinterpret_cast<const char*>(frame.data() + sentTotal),
                              chunk, 0);
        if (sent <= 0) {
            closesocket(clientSocket_);
            clientSocket_ = INVALID_SOCKET;
            clientConnected_ = false;
            break;
        }
        sentTotal += static_cast<size_t>(sent);
    }
}

void VideoStreamServer::stop() {
    if (!running_.exchange(false)) return;

    if (listenSocket_ != INVALID_SOCKET) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }

    std::lock_guard lock(socketMutex_);
    if (clientSocket_ != INVALID_SOCKET) {
        closesocket(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
    }
    clientConnected_ = false;

    if (acceptThread_ && acceptThread_->joinable()) {
        // accept() is released by closing the listening socket.
        acceptThread_->join();
    }
    acceptThread_.reset();
}

}
