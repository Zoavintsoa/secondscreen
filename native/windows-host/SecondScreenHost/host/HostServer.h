#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include "DiscoveryService.h"
#include "PairingManager.h"

namespace second_screen {

class HostServer {
public:
    HostServer();
    ~HostServer();

    bool start();
    void stop();

private:
    std::atomic_bool running_{false};
    std::unique_ptr<std::thread> discoveryThread_;
    std::unique_ptr<std::thread> controlThread_;
    DiscoveryService discovery_;
};

}
