// SecondScreen Windows Native Host Network Server Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "NetworkServer.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <array>
#include <cstring>

namespace SecondScreen {

    NetworkServer::NetworkServer()
        : m_State(ServerState::STOPPED),
          m_ControlPort(DEFAULT_HOST_PORT),
          m_ControlListenSocket(INVALID_SOCKET),
          m_DiscoverySocket(INVALID_SOCKET),
          m_VideoListenSocket(INVALID_SOCKET),
          m_Running(false),
          m_PacketSequence(0) {
    }

    NetworkServer::~NetworkServer() {
        Stop();
    }

    bool NetworkServer::InitializeWinSock() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "[NetworkServer] WSAStartup failed: " << result << std::endl;
            return false;
        }
        return true;
    }

    bool NetworkServer::Start(uint16_t controlPort, const std::string& expectedPin) {
        if (m_Running) return true;

        if (!InitializeWinSock()) return false;

        m_ControlPort = controlPort;
        m_PairingPin = expectedPin;
        m_Running = true;

        // 1. Setup TCP Control Listen Socket
        m_ControlListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_ControlListenSocket == INVALID_SOCKET) {
            std::cerr << "[NetworkServer] Failed to create control socket." << std::endl;
            return false;
        }

        // Enable SO_REUSEADDR and TCP_NODELAY (disable Nagle's algorithm for low latency)
        int opt = 1;
        setsockopt(m_ControlListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(controlPort);

        if (bind(m_ControlListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "[NetworkServer] Failed to bind control socket to port " << controlPort << std::endl;
            closesocket(m_ControlListenSocket);
            m_ControlListenSocket = INVALID_SOCKET;
            return false;
        }

        if (listen(m_ControlListenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "[NetworkServer] Failed to listen on control socket." << std::endl;
            closesocket(m_ControlListenSocket);
            m_ControlListenSocket = INVALID_SOCKET;
            return false;
        }

        m_State = ServerState::LISTENING;
        std::cout << "[NetworkServer] Control channel listening on TCP port " << controlPort << std::endl;

        // Launch worker threads
        m_ControlThread = std::thread(&NetworkServer::ControlListenLoop, this);
        m_DiscoveryThread = std::thread(&NetworkServer::DiscoveryListenLoop, this);

        return true;
    }

    void NetworkServer::Stop() {
        if (!m_Running) return;
        m_Running = false;
        m_State = ServerState::STOPPED;

        if (m_ControlListenSocket != INVALID_SOCKET) {
            closesocket(m_ControlListenSocket);
            m_ControlListenSocket = INVALID_SOCKET;
        }
        if (m_DiscoverySocket != INVALID_SOCKET) {
            closesocket(m_DiscoverySocket);
            m_DiscoverySocket = INVALID_SOCKET;
        }

        {
            std::lock_guard<std::mutex> lock(m_ClientMutex);
            if (m_ConnectedClient.controlSocket != INVALID_SOCKET) {
                closesocket(m_ConnectedClient.controlSocket);
                m_ConnectedClient.controlSocket = INVALID_SOCKET;
            }
            if (m_ConnectedClient.videoSocket != INVALID_SOCKET) {
                closesocket(m_ConnectedClient.videoSocket);
                m_ConnectedClient.videoSocket = INVALID_SOCKET;
            }
            m_ConnectedClient.isAuthenticated = false;
        }

        if (m_ControlThread.joinable()) m_ControlThread.join();
        if (m_DiscoveryThread.joinable()) m_DiscoveryThread.join();

        WSACleanup();
        std::cout << "[NetworkServer] Server stopped." << std::endl;
    }

    void NetworkServer::ControlListenLoop() {
        while (m_Running) {
            sockaddr_in clientAddr = {};
            int addrLen = sizeof(clientAddr);

            SOCKET clientSock = accept(m_ControlListenSocket, (sockaddr*)&clientAddr, &addrLen);
            if (clientSock == INVALID_SOCKET) {
                if (!m_Running) break;
                continue;
            }

            // Set TCP_NODELAY
            int flag = 1;
            setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));

            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
            std::cout << "[NetworkServer] New client connection from " << ipStr << ":" << ntohs(clientAddr.sin_port) << std::endl;

            {
                std::lock_guard<std::mutex> lock(m_ClientMutex);
                m_ConnectedClient.controlSocket = clientSock;
                m_ConnectedClient.clientAddr = clientAddr;
                m_ConnectedClient.isAuthenticated = false;
                m_State = ServerState::CLIENT_CONNECTED;
            }

            std::vector<uint8_t> receiveBuffer;
            std::array<uint8_t, 4096> readBuffer = {};
            while (m_Running) {
                int bytesRead = recv(clientSock, reinterpret_cast<char*>(readBuffer.data()), static_cast<int>(readBuffer.size()), 0);
                if (bytesRead <= 0) {
                    std::cout << "[NetworkServer] Client disconnected." << std::endl;
                    break;
                }
                receiveBuffer.insert(receiveBuffer.end(), readBuffer.begin(), readBuffer.begin() + bytesRead);
                if (!ProcessControlBuffer(clientSock, &receiveBuffer)) {
                    std::cerr << "[NetworkServer] Closing client connection after invalid control packet." << std::endl;
                    break;
                }
            }

            {
                std::lock_guard<std::mutex> lock(m_ClientMutex);
                closesocket(clientSock);
                m_ConnectedClient.controlSocket = INVALID_SOCKET;
                m_ConnectedClient.isAuthenticated = false;
                if (m_Running) m_State = ServerState::LISTENING;
            }
        }
    }

    void NetworkServer::DiscoveryListenLoop() {
        m_DiscoverySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_DiscoverySocket == INVALID_SOCKET) return;

        int opt = 1;
        setsockopt(m_DiscoverySocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in discAddr = {};
        discAddr.sin_family = AF_INET;
        discAddr.sin_addr.s_addr = INADDR_ANY;
        discAddr.sin_port = htons(DEFAULT_DISCOVERY_PORT);

        if (bind(m_DiscoverySocket, (sockaddr*)&discAddr, sizeof(discAddr)) == SOCKET_ERROR) {
            closesocket(m_DiscoverySocket);
            m_DiscoverySocket = INVALID_SOCKET;
            return;
        }

        std::vector<uint8_t> buffer(1024);
        while (m_Running) {
            sockaddr_in senderAddr = {};
            int senderLen = sizeof(senderAddr);
            int bytes = recvfrom(m_DiscoverySocket, (char*)buffer.data(), (int)buffer.size(), 0, (sockaddr*)&senderAddr, &senderLen);
            if (bytes <= 0 || !m_Running) break;

            // Respond with SecondScreen Host Beacon
            std::string response = "{\"type\":\"DISCOVER_RESPONSE\",\"version\":1,\"port\":" + std::to_string(m_ControlPort) + ",\"name\":\"Windows-SecondScreen-Host\"}";
            sendto(m_DiscoverySocket, response.c_str(), (int)response.length(), 0, (sockaddr*)&senderAddr, senderLen);
        }
    }

    bool NetworkServer::ProcessControlBuffer(SOCKET clientSock, std::vector<uint8_t>* receiveBuffer) {
        while (receiveBuffer->size() >= sizeof(PacketHeader)) {
            PacketHeader header = {};
            std::memcpy(&header, receiveBuffer->data(), sizeof(header));
            if (header.magic != PROTOCOL_MAGIC || header.version != PROTOCOL_VERSION || header.payloadSize > MAX_CONTROL_PAYLOAD_SIZE) {
                return false;
            }

            const size_t packetSize = sizeof(PacketHeader) + static_cast<size_t>(header.payloadSize);
            if (receiveBuffer->size() < packetSize) {
                return true;
            }

            const uint8_t* payload = header.payloadSize == 0 ? nullptr : receiveBuffer->data() + sizeof(PacketHeader);
            if (!ProcessControlMessage(clientSock, header, payload)) {
                return false;
            }
            receiveBuffer->erase(receiveBuffer->begin(), receiveBuffer->begin() + packetSize);
        }
        return true;
    }

    bool NetworkServer::ProcessControlMessage(SOCKET clientSock, const PacketHeader& header, const uint8_t* payload) {
        const auto messageType = static_cast<MessageType>(header.type);
        if (messageType == MessageType::PAIR_REQUEST) {
            if (header.payloadSize == 0 || header.payloadSize > 32) {
                return false;
            }

            const std::string receivedPin(reinterpret_cast<const char*>(payload), header.payloadSize);
            if (!ConstantTimeEquals(receivedPin, m_PairingPin)) {
                std::cerr << "[NetworkServer] Rejected pairing request with invalid PIN." << std::endl;
                return false;
            }

            std::lock_guard<std::mutex> lock(m_ClientMutex);
            if (m_ConnectedClient.controlSocket != clientSock) {
                return false;
            }
            m_ConnectedClient.isAuthenticated = true;
            m_ConnectedClient.sessionId = "sess_win_" + std::to_string(GetTickCount64());
            m_ConnectedClient.sessionToken = m_ConnectedClient.sessionId + "_" + std::to_string(++m_PacketSequence);
            m_State = ServerState::STREAMING;
            if (!SendPacket(clientSock, MessageType::PAIR_RESPONSE, 0, 0,
                            reinterpret_cast<const uint8_t*>(m_ConnectedClient.sessionToken.data()),
                            static_cast<uint32_t>(m_ConnectedClient.sessionToken.size()))) {
                return false;
            }
            std::cout << "[NetworkServer] Client authenticated. Streaming enabled for this session." << std::endl;
            return true;
        }

        if (messageType == MessageType::INPUT_EVENT) {
            if (header.payloadSize != sizeof(InputEvent)) {
                return false;
            }
            std::lock_guard<std::mutex> lock(m_ClientMutex);
            if (!m_ConnectedClient.isAuthenticated || m_ConnectedClient.controlSocket != clientSock) {
                return false;
            }
            InputEvent input = {};
            std::memcpy(&input, payload, sizeof(input));
            if (m_InputCallback) {
                m_InputCallback(input);
            }
            return true;
        }

        if (messageType == MessageType::PING) {
            std::lock_guard<std::mutex> lock(m_ClientMutex);
            if (!m_ConnectedClient.isAuthenticated || m_ConnectedClient.controlSocket != clientSock) {
                return false;
            }
            return SendPacket(clientSock, MessageType::PONG, 0, header.timestamp, nullptr, 0);
        }

        return messageType == MessageType::DISCONNECT && header.payloadSize == 0;
    }

    static bool SendAll(SOCKET sock, const char* data, int totalBytes) {
        int bytesSent = 0;
        while (bytesSent < totalBytes) {
            int res = send(sock, data + bytesSent, totalBytes - bytesSent, 0);
            if (res <= 0) return false;
            bytesSent += res;
        }
        return true;
    }

    bool NetworkServer::SendPacket(SOCKET socket, MessageType type, uint8_t flags, uint64_t timestamp, const uint8_t* payload, uint32_t payloadSize) {
        PacketHeader header = {};
        header.magic = PROTOCOL_MAGIC;
        header.version = PROTOCOL_VERSION;
        header.type = static_cast<uint8_t>(type);
        header.flags = flags;
        header.sequence = ++m_PacketSequence;
        header.timestamp = timestamp;
        header.payloadSize = payloadSize;
        if (!SendAll(socket, reinterpret_cast<const char*>(&header), sizeof(header))) {
            return false;
        }
        return payloadSize == 0 || SendAll(socket, reinterpret_cast<const char*>(payload), static_cast<int>(payloadSize));
    }

    bool NetworkServer::ConstantTimeEquals(const std::string& left, const std::string& right) {
        const size_t maxSize = std::max(left.size(), right.size());
        unsigned char difference = static_cast<unsigned char>(left.size() ^ right.size());
        for (size_t i = 0; i < maxSize; ++i) {
            const unsigned char leftByte = i < left.size() ? static_cast<unsigned char>(left[i]) : 0;
            const unsigned char rightByte = i < right.size() ? static_cast<unsigned char>(right[i]) : 0;
            difference |= static_cast<unsigned char>(leftByte ^ rightByte);
        }
        return difference == 0;
    }

    bool NetworkServer::SendVideoFrame(const EncodedVideoPacket& packet) {
        std::lock_guard<std::mutex> lock(m_ClientMutex);
        if (!m_ConnectedClient.isAuthenticated || m_ConnectedClient.controlSocket == INVALID_SOCKET) {
            return false;
        }

        if (packet.data.size() > MAX_VIDEO_PAYLOAD_SIZE) {
            return false;
        }
        return SendPacket(m_ConnectedClient.controlSocket, MessageType::FRAME, packet.isKeyFrame ? 0x01 : 0x00,
                          packet.captureTimestampUs, packet.data.data(), static_cast<uint32_t>(packet.data.size()));
    }

    bool NetworkServer::SendTelemetry(const TelemetryData& telemetry) {
        std::lock_guard<std::mutex> lock(m_ClientMutex);
        if (!m_ConnectedClient.isAuthenticated || m_ConnectedClient.controlSocket == INVALID_SOCKET) {
            return false;
        }

        return SendPacket(m_ConnectedClient.controlSocket, MessageType::TELEMETRY, 0, 0,
                          reinterpret_cast<const uint8_t*>(&telemetry), sizeof(telemetry));
    }

}
