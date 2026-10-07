#include "HostServer.h"
#include <winsock2.h>
#include <ws2tcpip.h>

namespace second_screen {

HostServer::HostServer() = default;

HostServer::~HostServer() {
    stop();
}

bool HostServer::start() {
    if (running_.exchange(true)) {
        return true;
    }

    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        running_ = false;
        return false;
    }

    // Discovery/control/stream services are intentionally separated from
    // the IddCx driver. This keeps network failures out of the display path.
    return true;
}

void HostServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    controlThread_.reset();
    discoveryThread_.reset();
    WSACleanup();
}

}
