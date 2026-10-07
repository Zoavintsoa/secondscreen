#pragma once

#include <atomic>
#include <memory>
#include <thread>

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
};

}
