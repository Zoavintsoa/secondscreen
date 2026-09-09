// SecondScreen Windows Host Engine
// Orchestrator: DXGI Capture -> GPU Texture -> NVENC Hardware Encoder -> Packetizer -> WinSock2 Server
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#include "../capture/DXGICapture.h"
#include "../encoder/HardwareEncoder.h"
#include "../encoder/NVENCEncoder.h"
#include "../network/NetworkServer.h"
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

namespace SecondScreen {

    struct HostEngineConfig {
        UINT displayIndex = 0;
        bool requireSecondScreenVirtualDisplay = true;
        UINT width = 1920;
        UINT height = 1080;
        UINT fps = 60;
        UINT bitrateKbps = 8000;        // 8 Mbps default as requested
        UINT maxBitrateKbps = 10000;     // 10 Mbps ceiling
        UINT gopSize = 60;               // 1s GOP for 60fps
        uint16_t port = DEFAULT_HOST_PORT;
        std::string pairingPin = "123456";
    };

    class HostEngine {
    public:
        HostEngine();
        ~HostEngine();

        // Print formatted GPU detection banner & probe NVENC
        bool DetectAndPrintCapabilities();

        // Initialize capture, encoder, network server, and input injection
        bool Start(const HostEngineConfig& config);

        // Stop streaming and release all resources
        void Stop();

        bool IsRunning() const { return m_Running; }
        const HostEngineConfig& GetConfig() const { return m_Config; }

    private:
        void StreamWorkerLoop();
        void HandleClientInput(const InputEvent& event);
        void InjectInputWindows(const InputEvent& event);
        bool InitializeCaptureTarget();

        HostEngineConfig m_Config;
        DXGICaptureManager m_Capture;
        std::unique_ptr<IHardwareEncoder> m_Encoder;
        NetworkServer m_Server;

        std::thread m_StreamThread;
        std::atomic<bool> m_Running;

        // Telemetry tracking
        uint64_t m_CapturedFramesCount;
        uint64_t m_EncodedFramesCount;
        uint64_t m_SentFramesCount;
        uint64_t m_DroppedFramesCount;
        std::chrono::high_resolution_clock::time_point m_LastTelemetryTime;
    };

}
