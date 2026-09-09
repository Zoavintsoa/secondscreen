// SecondScreen Windows Native Host Network Server
// High-performance asynchronous control & binary video streaming server (WinSock2)
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "../../../core/protocol/Protocol.h"
#include "../encoder/HardwareEncoder.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace SecondScreen {

    enum class ServerState {
        STOPPED = 0,
        LISTENING = 1,
        CLIENT_CONNECTED = 2,
        STREAMING = 3,
        ERROR_STATE = 4
    };

    struct ConnectedClientInfo {
        SOCKET controlSocket = INVALID_SOCKET;
        SOCKET videoSocket = INVALID_SOCKET;
        sockaddr_in clientAddr = {};
        std::string clientName;
        std::string platform;
        std::string sessionId;
        std::string sessionToken;
        bool isAuthenticated = false;
        uint64_t lastPingTime = 0;
        float measuredRttMs = 0.0f;
    };

    class NetworkServer {
    public:
        NetworkServer();
        ~NetworkServer();

        // Start Control Server (TCP 9876), Discovery Responder (UDP 9877), and Video Server (TCP/UDP 9878)
        bool Start(uint16_t controlPort = DEFAULT_HOST_PORT, const std::string& expectedPin = "123456");
        void Stop();

        // Send an encoded video frame to the authenticated streaming client
        bool SendVideoFrame(const EncodedVideoPacket& packet);

        // Send telemetry payload to client
        bool SendTelemetry(const TelemetryData& telemetry);

        // Callback for input events received from the client digitizer (Touch / Stylus)
        void SetInputEventCallback(std::function<void(const InputEvent&)> callback) {
            m_InputCallback = callback;
        }

        ServerState GetState() const { return m_State; }
        bool HasStreamingClient() const { return m_State == ServerState::STREAMING; }
        const ConnectedClientInfo& GetClientInfo() const { return m_ConnectedClient; }

    private:
        bool InitializeWinSock();
        void ControlListenLoop();
        void DiscoveryListenLoop();
        bool ProcessControlBuffer(SOCKET clientSock, std::vector<uint8_t>* receiveBuffer);
        bool ProcessControlMessage(SOCKET clientSock, const PacketHeader& header, const uint8_t* payload);
        bool SendPacket(SOCKET socket, MessageType type, uint8_t flags, uint64_t timestamp, const uint8_t* payload, uint32_t payloadSize);
        static bool ConstantTimeEquals(const std::string& left, const std::string& right);

        std::atomic<ServerState> m_State;
        std::string m_PairingPin;
        uint16_t m_ControlPort;
        SOCKET m_ControlListenSocket;
        SOCKET m_DiscoverySocket;
        SOCKET m_VideoListenSocket;

        ConnectedClientInfo m_ConnectedClient;
        std::mutex m_ClientMutex;

        std::thread m_ControlThread;
        std::thread m_DiscoveryThread;
        std::atomic<bool> m_Running;
        uint32_t m_PacketSequence;

        std::function<void(const InputEvent&)> m_InputCallback;
    };

}
