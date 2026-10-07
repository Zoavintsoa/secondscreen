#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <winsock2.h>

namespace second_screen {
class VideoStreamServer {
public:
    ~VideoStreamServer();
    bool start(uint16_t port);
    void stop();
    bool hasClient() const;
    void setAuthorized(bool authorized);
    bool isAuthorized() const;
    void sendFrame(const std::vector<uint8_t>& annexB, bool keyFrame, uint64_t timestampUs);
private:
    void acceptLoop();
    std::atomic_bool running_{false};
    std::atomic_bool authorized_{false};
    std::atomic_bool clientConnected_{false};
    SOCKET listenSocket_{INVALID_SOCKET};
    SOCKET clientSocket_{INVALID_SOCKET};
    std::unique_ptr<std::thread> acceptThread_;
    mutable std::mutex socketMutex_;
};
}
