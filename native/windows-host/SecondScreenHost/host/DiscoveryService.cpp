#include "DiscoveryService.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

namespace second_screen {
DiscoveryService::~DiscoveryService(){ stop(); }

bool DiscoveryService::start(const std::string& hostName, uint16_t controlPort) {
    if (running_.exchange(true)) return true;
    hostName_ = hostName;
    controlPort_ = controlPort;
    thread_ = std::thread([this] {
        SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == INVALID_SOCKET) { running_ = false; return; }
        BOOL yes = TRUE;
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));
        sockaddr_in target{};
        target.sin_family = AF_INET;
        target.sin_port = htons(49151);
        target.sin_addr.s_addr = INADDR_BROADCAST;

        const std::string payload =
            "{\"service\":\"secondscreen\",\"protocol\":1,\"name\":\"" + hostName_ +
            "\",\"controlPort\":" + std::to_string(controlPort_) + "}";

        while (running_) {
            sendto(s, payload.data(), static_cast<int>(payload.size()), 0,
                   reinterpret_cast<sockaddr*>(&target), sizeof(target));
            Sleep(1000);
        }
        closesocket(s);
    });
    return true;
}
void DiscoveryService::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}
}
