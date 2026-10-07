#pragma once
#include <atomic>
#include <string>
#include <thread>

namespace second_screen {
class DiscoveryService {
public:
    DiscoveryService() = default;
    ~DiscoveryService();
    bool start(const std::string& hostName, uint16_t controlPort);
    void stop();
private:
    std::atomic_bool running_{false};
    std::thread thread_;
    std::string hostName_;
    uint16_t controlPort_{};
};
}
