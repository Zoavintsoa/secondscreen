// SecondScreen Windows Native Host Entry Point
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "host/HostEngine.h"
#include <iostream>
#include <string>
#include <csignal>
#include <atomic>

static std::atomic<bool> g_KeepRunning(true);

#ifdef _WIN32
static BOOL WINAPI ConsoleCtrlHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_BREAK_EVENT) {
        std::cout << "\n[SecondScreen Host] Shutting down..." << std::endl;
        g_KeepRunning = false;
        return TRUE;
    }
    return FALSE;
}
#endif

int main(int argc, char* argv[]) {
    std::cout << "SecondScreen Windows Native Host v1.0" << std::endl;

#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#endif

    SecondScreen::HostEngineConfig config = {};
    config.requireSecondScreenVirtualDisplay = true;
    config.width = 1920;
    config.height = 1080;
    config.fps = 60;
    config.bitrateKbps = 8000;
    config.maxBitrateKbps = 10000;
    config.gopSize = 60;
    config.port = SecondScreen::DEFAULT_HOST_PORT;
    config.pairingPin = "123456";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--display-index" && i + 1 < argc) {
            config.displayIndex = std::stoi(argv[++i]);
            config.requireSecondScreenVirtualDisplay = false;
        } else if (arg == "--fps" && i + 1 < argc) {
            config.fps = std::stoi(argv[++i]);
        } else if (arg == "--bitrate" && i + 1 < argc) {
            config.bitrateKbps = std::stoi(argv[++i]);
            config.maxBitrateKbps = config.bitrateKbps + 2000;
        } else if (arg == "--port" && i + 1 < argc) {
            config.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--pin" && i + 1 < argc) {
            config.pairingPin = argv[++i];
        } else if (arg == "--res" && i + 1 < argc) {
            std::string res = argv[++i];
            size_t xPos = res.find('x');
            if (xPos != std::string::npos) {
                config.width = std::stoi(res.substr(0, xPos));
                config.height = std::stoi(res.substr(xPos + 1));
            }
        }
    }

    SecondScreen::HostEngine engine;

    if (!engine.Start(config)) {
        std::cerr << "[SecondScreen Host] Failed to start engine." << std::endl;
        return 1;
    }

    // Keep running until signal
    while (g_KeepRunning && engine.IsRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    engine.Stop();
    std::cout << "[SecondScreen Host] Terminated cleanly." << std::endl;
    return 0;
}
