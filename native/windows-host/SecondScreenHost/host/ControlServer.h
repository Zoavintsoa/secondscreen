#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <winsock2.h>

#include "PairingManager.h"

namespace second_screen {

class ControlServer {
public:
    using AuthCallback = std::function<void(bool)>;
    using KeyframeCallback = std::function<void()>;

    ControlServer(PairingManager& pairing, AuthCallback authCallback, KeyframeCallback keyframeCallback);
    ~ControlServer();

    bool start(uint16_t port);
    void stop();

private:
    void acceptLoop();
    void clientLoop(SOCKET client);

    PairingManager& pairing_;
    AuthCallback authCallback_;
    KeyframeCallback keyframeCallback_;

    std::atomic_bool running_{false};
    SOCKET listenSocket_{INVALID_SOCKET};
    std::unique_ptr<std::thread> acceptThread_;
    std::mutex clientMutex_;
    SOCKET clientSocket_{INVALID_SOCKET};
};

}
