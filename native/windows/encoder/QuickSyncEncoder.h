// SecondScreen Intel QuickSync Video Hardware Encoder
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#include "HardwareEncoder.h"

namespace SecondScreen {

    class QuickSyncEncoder : public IHardwareEncoder {
    public:
        QuickSyncEncoder();
        virtual ~QuickSyncEncoder();

        static bool IsSupported(ID3D11Device* pDevice);

        virtual bool Initialize(ID3D11Device* pDevice, const EncoderConfig& config) override;
        virtual bool Reconfigure(const EncoderConfig& config) override;
        virtual bool EncodeTexture(ID3D11Texture2D* pTexture, uint64_t frameNumber, uint64_t captureTimestampUs, bool forceKeyFrame, EncodedVideoPacket* pOutPacket) override;
        virtual void Flush(std::vector<EncodedVideoPacket>& outPackets) override;
        virtual void Shutdown() override;

        virtual EncoderBackend GetBackendType() const override { return EncoderBackend::QUICKSYNC; }
        virtual const char* GetBackendName() const override { return "Intel QuickSync (Hardware Accelerated)"; }
        virtual bool IsHardwareAccelerated() const override { return true; }

    private:
        HMODULE m_hMfxDll;
        EncoderConfig m_Config;
        bool m_Initialized;
    };

}
