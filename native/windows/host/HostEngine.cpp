// SecondScreen Windows Host Engine Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "HostEngine.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace SecondScreen {

    HostEngine::HostEngine()
        : m_Running(false),
          m_CapturedFramesCount(0),
          m_EncodedFramesCount(0),
          m_SentFramesCount(0),
          m_DroppedFramesCount(0),
          m_LastTelemetryTime(std::chrono::high_resolution_clock::now()) {
    }

    HostEngine::~HostEngine() {
        Stop();
    }

    bool HostEngine::DetectAndPrintCapabilities() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "SecondScreen Host" << std::endl;
        std::cout << "========================================" << std::endl;

        auto displays = DXGICaptureManager::EnumerateDisplays();
        if (displays.empty()) {
            std::cout << "GPU: No compatible DXGI adapter found." << std::endl;
            std::cout << "NVENC: Unavailable" << std::endl;
            std::cout << "Reason: No DirectX 11 hardware adapter detected." << std::endl;
            return false;
        }

        const auto& mainDisplay = displays[0];
        std::wstring adapterName = mainDisplay.adapterDescription;
        std::string adapterStr(adapterName.begin(), adapterName.end());

        std::cout << "GPU: " << adapterStr << std::endl;

        // Initialize temporary DXGI capture to probe D3D11 device & NVENC support
        DXGICaptureManager probeCapture;
        if (!probeCapture.InitializeByDisplayName(mainDisplay.deviceName)) {
            std::cout << "NVENC: Unavailable" << std::endl;
            std::cout << "Reason: Failed to initialize DXGI capture for the selected DXGI adapter." << std::endl;
            return false;
        }

        bool nvencSupported = NVENCEncoder::IsSupported(probeCapture.GetDevice());
        if (nvencSupported) {
            std::cout << "NVENC: Available" << std::endl;
            std::cout << "Capture: DXGI Desktop Duplication" << std::endl;
            std::cout << "Codec: H.264" << std::endl;
            std::cout << "Resolution: " << mainDisplay.width << "x" << mainDisplay.height << std::endl;
            std::cout << "FPS: 60" << std::endl;
            std::cout << "Bitrate: 8 Mbps" << std::endl;
        } else {
            std::cout << "NVENC: Unavailable" << std::endl;
            std::cout << "Reason: NVIDIA nvEncodeAPI64.dll or supported NVENC hardware session not found." << std::endl;
        }

        std::cout << "========================================\n" << std::endl;
        return nvencSupported;
    }

    bool HostEngine::Start(const HostEngineConfig& config) {
        if (m_Running) return true;
        m_Config = config;

        // 1. Detect capabilities and output banner
        DetectAndPrintCapabilities();

        if (!InitializeCaptureTarget()) {
            std::cerr << "[HostEngine] Failed to initialize the requested capture target." << std::endl;
            return false;
        }

        const auto& captureInfo = m_Capture.GetCurrentDisplayInfo();
        if (captureInfo.width != config.width || captureInfo.height != config.height) {
            std::cerr << "[HostEngine] Requested encoder resolution " << config.width << "x" << config.height
                      << " does not match the selected display " << captureInfo.width << "x" << captureInfo.height << std::endl;
            m_Capture.Cleanup();
            return false;
        }

        // 3. Initialize Hardware Encoder (NVIDIA NVENC)
        EncoderConfig encConfig = {};
        encConfig.codec = CodecType::H264;
        encConfig.width = config.width;
        encConfig.height = config.height;
        encConfig.fps = config.fps;
        encConfig.bitrateKbps = config.bitrateKbps;
        encConfig.maxBitrateKbps = config.maxBitrateKbps;
        encConfig.gopSize = config.gopSize;
        encConfig.rateControl = RateControlMode::CBR;
        encConfig.lowLatencyMode = true;

        m_Encoder = HardwareEncoderFactory::CreateEncoder(m_Capture.GetDevice(), encConfig);
        if (!m_Encoder) {
            std::cerr << "[HostEngine] Failed to initialize a hardware encoder." << std::endl;
            m_Capture.Cleanup();
            return false;
        }

        // 4. Hook up Input Callback
        m_Server.SetInputEventCallback([this](const InputEvent& event) {
            this->HandleClientInput(event);
        });

        // 5. Start Network Server (TCP 9876, UDP 9877)
        if (!m_Server.Start(config.port, config.pairingPin)) {
            std::cerr << "[HostEngine] Failed to start network server on port " << config.port << std::endl;
            m_Encoder->Shutdown();
            m_Capture.Cleanup();
            return false;
        }

        // 6. Launch low-latency streaming thread
        m_Running = true;
        m_CapturedFramesCount = 0;
        m_EncodedFramesCount = 0;
        m_SentFramesCount = 0;
        m_DroppedFramesCount = 0;
        m_LastTelemetryTime = std::chrono::high_resolution_clock::now();

        m_StreamThread = std::thread(&HostEngine::StreamWorkerLoop, this);

        std::cout << "[HostEngine] SecondScreen Host running. Ready for iPad / Client connections." << std::endl;
        return true;
    }

    bool HostEngine::InitializeCaptureTarget() {
        if (m_Config.requireSecondScreenVirtualDisplay) {
            return m_Capture.InitializeSecondScreenVirtualDisplay();
        }
        return m_Capture.Initialize(m_Config.displayIndex);
    }

    void HostEngine::Stop() {
        if (!m_Running) return;
        m_Running = false;

        if (m_StreamThread.joinable()) {
            m_StreamThread.join();
        }

        m_Server.Stop();

        if (m_Encoder) {
            m_Encoder->Shutdown();
            m_Encoder.reset();
        }

        m_Capture.Cleanup();
        std::cout << "[HostEngine] Host engine stopped." << std::endl;
    }

    void HostEngine::StreamWorkerLoop() {
        const auto frameDuration = std::chrono::microseconds(1000000 / m_Config.fps);

        while (m_Running) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            if (!m_Server.HasStreamingClient()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            // 1. Acquire frame from DXGI (Direct VRAM GPU texture)
            CapturedGPUFrame gpuFrame = {};
            if (m_Capture.AcquireNextFrame(&gpuFrame, 16)) {
                m_CapturedFramesCount++;

                // 2. Encode frame directly via NVENC (Zero-copy GPU texture -> H.264 Annex-B)
                EncodedVideoPacket packet = {};
                if (m_Encoder->EncodeTexture(gpuFrame.texture.Get(), gpuFrame.frameSequenceNumber, gpuFrame.captureTimestampUs, gpuFrame.isKeyFrameRequired, &packet)) {
                    m_EncodedFramesCount++;

                    // 3. Send over TCP with immediate framing
                    if (m_Server.SendVideoFrame(packet)) {
                        m_SentFramesCount++;
                    } else {
                        // Drop frame if network buffer is congested to avoid latency buildup
                        m_DroppedFramesCount++;
                    }
                }
            }

            // 4. Periodically compute and send real telemetry to iPad HUD (every 1 second)
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastTelemetryTime).count();
            if (elapsed >= 1000) {
                float fps = (float)m_SentFramesCount * 1000.0f / (float)elapsed;
                float bitrateMbps = (float)(m_Config.bitrateKbps) / 1000.0f;

                TelemetryData telem = {};
                telem.fps = fps;
                telem.bitrateMbps = bitrateMbps;
                telem.rttMs = m_Server.GetClientInfo().measuredRttMs;
                telem.decodeLatencyMs = 0.0f;
                telem.renderLatencyMs = 0.0f;
                telem.totalLatencyMs = telem.rttMs;
                telem.packetLossPercent = 0.0f;
                telem.frameDrops = (uint32_t)m_DroppedFramesCount;
                telem.jitterMs = 0.0f;
                telem.isMeasured = 0;

                m_Server.SendTelemetry(telem);

                m_SentFramesCount = 0;
                m_LastTelemetryTime = now;
            }

            // Frame rate limiter
            auto workDuration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frameStart);
            if (workDuration < frameDuration) {
                std::this_thread::sleep_for(frameDuration - workDuration);
            }
        }
    }

    void HostEngine::HandleClientInput(const InputEvent& event) {
        InjectInputWindows(event);
    }

    void HostEngine::InjectInputWindows(const InputEvent& event) {
        const auto& display = m_Capture.GetCurrentDisplayInfo();
        const auto bounds = display.desktopCoordinates;
        const LONG pixelX = bounds.left + static_cast<LONG>(event.normX * (bounds.right - bounds.left));
        const LONG pixelY = bounds.top + static_cast<LONG>(event.normY * (bounds.bottom - bounds.top));
        const int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
        const int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
        const int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        if (virtualWidth <= 0 || virtualHeight <= 0) {
            return;
        }
        const int absX = static_cast<int>((static_cast<double>(pixelX - virtualLeft) * 65535.0) / virtualWidth);
        const int absY = static_cast<int>((static_cast<double>(pixelY - virtualTop) * 65535.0) / virtualHeight);

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = absX;
        input.mi.dy = absY;

        switch (event.actionType) {
            case 0: // DOWN (Touch / Click)
                input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
                if (event.button == 2) {
                    input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
                } else if (event.button == 1) {
                    input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
                } else {
                    input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
                }
                break;

            case 1: // MOVE (Touch Move / Cursor Hover)
                input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
                break;

            case 2: // UP (Touch Release / Click Up)
                input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
                if (event.button == 2) {
                    input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
                } else if (event.button == 1) {
                    input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
                } else {
                    input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
                }
                break;

            case 3: // WHEEL SCROLL
                input.mi.dwFlags = MOUSEEVENTF_WHEEL;
                input.mi.mouseData = static_cast<DWORD>(event.normY * 120.0f);
                break;

            case 4: // STYLUS / APPLE PENCIL
                input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN;
                break;

            default:
                return;
        }

        ::SendInput(1, &input, sizeof(INPUT));
    }

}
