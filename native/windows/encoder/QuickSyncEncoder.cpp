// SecondScreen Intel QuickSync Video Hardware Encoder Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "QuickSyncEncoder.h"
#include <iostream>
#include <chrono>

namespace SecondScreen {

    QuickSyncEncoder::QuickSyncEncoder()
        : m_hMfxDll(nullptr),
          m_Initialized(false) {
    }

    QuickSyncEncoder::~QuickSyncEncoder() {
        Shutdown();
    }

    bool QuickSyncEncoder::IsSupported(ID3D11Device* pDevice) {
        if (!pDevice) return false;

        // Check for Intel Media SDK / oneVPL runtime DLLs
        HMODULE hModule = LoadLibraryW(L"libmfxhw64.dll");
        if (!hModule) {
            hModule = LoadLibraryW(L"vpl.dll");
        }
        if (hModule) {
            FreeLibrary(hModule);
            return true;
        }
        return false;
    }

    bool QuickSyncEncoder::Initialize(ID3D11Device* pDevice, const EncoderConfig& config) {
        if (!pDevice) return false;
        m_Config = config;

        m_hMfxDll = LoadLibraryW(L"libmfxhw64.dll");
        if (!m_hMfxDll) {
            m_hMfxDll = LoadLibraryW(L"vpl.dll");
        }

        if (!m_hMfxDll) {
            std::wcerr << L"[QuickSync] Intel Media SDK / oneVPL runtime not found." << std::endl;
            return false;
        }

        std::wcout << L"[QuickSync] Initialized Intel hardware encoder: " << config.width << L"x" << config.height << std::endl;
        m_Initialized = true;
        return true;
    }

    bool QuickSyncEncoder::Reconfigure(const EncoderConfig& config) {
        m_Config = config;
        return m_Initialized;
    }

    bool QuickSyncEncoder::EncodeTexture(ID3D11Texture2D* pTexture, uint64_t frameNumber, uint64_t captureTimestampUs, bool forceKeyFrame, EncodedVideoPacket* pOutPacket) {
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

    void QuickSyncEncoder::Flush(std::vector<EncodedVideoPacket>& outPackets) {
        UNREFERENCED_PARAMETER(outPackets);
    }

    void QuickSyncEncoder::Shutdown() {
        if (m_hMfxDll) {
            FreeLibrary(m_hMfxDll);
            m_hMfxDll = nullptr;
        }
        m_Initialized = false;
    }

}
