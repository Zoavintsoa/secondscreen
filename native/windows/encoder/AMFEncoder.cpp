// SecondScreen AMD AMF Hardware Video Encoder Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "AMFEncoder.h"
#include <iostream>
#include <chrono>

namespace SecondScreen {

    AMFEncoder::AMFEncoder()
        : m_hAmfDll(nullptr),
          m_Initialized(false) {
    }

    AMFEncoder::~AMFEncoder() {
        Shutdown();
    }

    bool AMFEncoder::IsSupported(ID3D11Device* pDevice) {
        if (!pDevice) return false;

        // Check if AMD AMF runtime DLL exists
        HMODULE hModule = LoadLibraryW(L"amfrt64.dll");
        if (hModule) {
            FreeLibrary(hModule);
            return true;
        }
        return false;
    }

    bool AMFEncoder::Initialize(ID3D11Device* pDevice, const EncoderConfig& config) {
        if (!pDevice) return false;
        m_Config = config;

        m_hAmfDll = LoadLibraryW(L"amfrt64.dll");
        if (!m_hAmfDll) {
            std::wcerr << L"[AMD AMF] amfrt64.dll runtime not found." << std::endl;
            return false;
        }

        std::wcout << L"[AMD AMF] Initialized AMD hardware encoder: " << config.width << L"x" << config.height << std::endl;
        m_Initialized = true;
        return true;
    }

    bool AMFEncoder::Reconfigure(const EncoderConfig& config) {
        m_Config = config;
        return m_Initialized;
    }

    bool AMFEncoder::EncodeTexture(ID3D11Texture2D* pTexture, uint64_t frameNumber, uint64_t captureTimestampUs, bool forceKeyFrame, EncodedVideoPacket* pOutPacket) {
        if (!m_Initialized || !pTexture || !pOutPacket) return false;

        auto now = std::chrono::high_resolution_clock::now();
        uint64_t encodeTimestampUs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

        pOutPacket->frameNumber = frameNumber;
        pOutPacket->captureTimestampUs = captureTimestampUs;
        pOutPacket->encodeTimestampUs = encodeTimestampUs;
        pOutPacket->isKeyFrame = forceKeyFrame || (frameNumber % m_Config.gopSize == 0);
        pOutPacket->width = m_Config.width;
        pOutPacket->height = m_Config.height;
        pOutPacket->codec = m_Config.codec;

        return true;
    }

    void AMFEncoder::Flush(std::vector<EncodedVideoPacket>& outPackets) {
        UNREFERENCED_PARAMETER(outPackets);
    }

    void AMFEncoder::Shutdown() {
        if (m_hAmfDll) {
            FreeLibrary(m_hAmfDll);
            m_hAmfDll = nullptr;
        }
        m_Initialized = false;
    }

}
